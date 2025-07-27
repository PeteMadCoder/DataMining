#include <iostream>
#include <queue>
#include <unordered_set>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include <cstring>
#include <regex>
#include <filesystem>

// Write callback for libcurl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append((char*)contents, total_size);
    return total_size;
}

// Download HTML from a URL
std::string download_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed: " << curl_easy_strerror(res) << std::endl;
        }
        
        curl_easy_cleanup(curl);
    }
    return response;
}

// Extract links from HTML using Gumbo-Parser
void extract_links(const std::string& html, const std::string& base_url, std::queue<std::string>& url_queue, std::unordered_set<std::string>& visited) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::queue<GumboNode*> nodes;
    nodes.push(output->root);
    
    while (!nodes.empty()) {
        GumboNode* node = nodes.front();
        nodes.pop();
        
        if (node->type != GUMBO_NODE_ELEMENT) continue;
        
        GumboAttribute* href = nullptr;
        if (node->v.element.tag == GUMBO_TAG_A) {
            href = gumbo_get_attribute(&node->v.element.attributes, "href");
        }
        
        // Process child nodes
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            nodes.push(static_cast<GumboNode*>(children->data[i]));
        }
        
        if (!href) continue;
        
        std::string link_url = href->value;
        
        // Skip invalid links
        if (link_url.empty() || link_url[0] == '#') continue;
        
        // Convert relative to absolute URL
        if (link_url.find("http") != 0) {
            if (link_url[0] == '/') {
                link_url = base_url + link_url;
            } else {
                link_url = base_url + "/" + link_url;
            }
        }
        
        // Add new URL to queue if not visited
        if (link_url.find(base_url) == 0 && visited.find(link_url) == visited.end()) {
            visited.insert(link_url);
            url_queue.push(link_url);
        }
    }
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}

std::string extract_base_domain(const std::string& url) {
    std::smatch match;
    std::regex domain_regex(R"(^(https?:\/\/[^\/]+))");
    if (std::regex_search(url, match, domain_regex)) {
        return match[1];
    }
    return "";
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <starting_url>" << std::endl;
        return 1;
    }
    
    std::string start_url = argv[1];
    std::string base_domain = extract_base_domain(start_url);
    std::queue<std::string> url_queue;
    std::unordered_set<std::string> visited;
    
    url_queue.push(start_url);
    visited.insert(start_url);
    
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // --- ensure output directory exists ---
    const std::string outDir = "output";
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);
    if (ec) {
        std::cerr << "Could not create output directory '" << outDir << "': " << ec.message() << "\n";
        return 1;
    }   
    
    
    while (!url_queue.empty()) {
        std::string url = url_queue.front();
        url_queue.pop();
        
        std::cout << "Downloading: " << url << std::endl;
        std::string html = download_url(url);
        
        if (!html.empty()) {
            // Save HTML to file
            // create a safe filename
            std::string safe = std::regex_replace(url, std::regex("[:/]+"), "_");
            std::string filename = outDir + "/" + safe + ".html";
            std::ofstream outfile(filename, std::ios::binary);
            outfile << html;
            outfile.close();
            
            // Extract links and add to queue
            extract_links(html, base_domain, url_queue, visited);
        }
    }
    
    curl_global_cleanup();
    return 0;
}