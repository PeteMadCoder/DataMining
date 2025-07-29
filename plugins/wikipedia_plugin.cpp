#include "plugin_interface.h" // Adjust path as needed
#include "processing/plugin_processor.h" // Adjust path as needed
// Include Gumbo for robust HTML parsing
#include <gumbo.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>

bool isHeading(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) return false;
    GumboTag tag = node->v.element.tag;
    return (tag == GUMBO_TAG_H1 || tag == GUMBO_TAG_H2 || tag == GUMBO_TAG_H3 ||
            tag == GUMBO_TAG_H4 || tag == GUMBO_TAG_H5 || tag == GUMBO_TAG_H6);
}

// Utility function to trim whitespace
std::string trim(const std::string& str) {
    if (str.empty()) return str;
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return ""; // No non-whitespace characters
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

// Utility function for basic HTML unescaping (extend as needed)
std::string unescapeHtml(const std::string& text) {
    if (text.empty()) return text;
    std::string result = text;
    // Order matters for &amp;
    result = std::regex_replace(result, std::regex("<"), "<");
    result = std::regex_replace(result, std::regex(">"), ">");
    result = std::regex_replace(result, std::regex("&quot;"), "\"");
    result = std::regex_replace(result, std::regex("&apos;"), "'");
    result = std::regex_replace(result, std::regex("&nbsp;"), " ");
    result = std::regex_replace(result, std::regex("&amp;"), "&"); // Do this last
    return result;
}

// Gumbo helper: Get text content from a GumboNode recursively
std::string getTextContent(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT &&
               node->v.element.tag != GUMBO_TAG_SCRIPT &&
               node->v.element.tag != GUMBO_TAG_STYLE) {
        std::string contents;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            const std::string& text = getTextContent(static_cast<GumboNode*>(children->data[i]));
            if (i != 0 && !text.empty()) {
                contents.append(" "); // Add space between text nodes
            }
            contents.append(text);
        }
        return contents;
    } else {
        return "";
    }
}

// Gumbo helper: Get the value of an attribute by name
const char* getAttributeValue(GumboNode* node, const char* attr_name) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return nullptr;
    }
    GumboAttribute* attr = gumbo_get_attribute(&node->v.element.attributes, attr_name);
    return attr ? attr->value : nullptr;
}

// Extractor for Wikipedia article title (from <h1 id="firstHeading">)
void extractWikipediaTitle(const std::string& html, ProcessedData& data) {
    GumboOutput* output = gumbo_parse(html.c_str());
    if (output && output->root) {
        // Find the node with id="firstHeading"
        std::function<GumboNode*(GumboNode*)> findFirstHeading = [&](GumboNode* node) -> GumboNode* {
            if (node->type != GUMBO_NODE_ELEMENT) return nullptr;
            
            const char* id = getAttributeValue(node, "id");
            if (id && std::string(id) == "firstHeading") {
                return node;
            }
            
            // Recursively search children
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* found = findFirstHeading(static_cast<GumboNode*>(children->data[i]));
                if (found) return found;
            }
            return nullptr;
        };

        GumboNode* heading_node = findFirstHeading(output->root);
        if (heading_node) {
            std::string title_text = getTextContent(heading_node);
            title_text = unescapeHtml(title_text);
            data.title = trim(title_text);
        }
    }
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
}

// Extractor for the main content text (first few paragraphs)
void extractWikipediaContent(const std::string& html, ProcessedData& data) {
    GumboOutput* output = gumbo_parse(html.c_str());
    if (output && output->root) {
        // Find the node with id="mw-content-text"
        std::function<GumboNode*(GumboNode*)> findContentText = [&](GumboNode* node) -> GumboNode* {
            if (node->type != GUMBO_NODE_ELEMENT) return nullptr;
            
            const char* id = getAttributeValue(node, "id");
            if (id && std::string(id) == "mw-content-text") {
                return node;
            }
            
            // Recursively search children
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* found = findContentText(static_cast<GumboNode*>(children->data[i]));
                if (found) return found;
            }
            return nullptr;
        };

        GumboNode* content_node = findContentText(output->root);
        if (content_node) {
            std::ostringstream content_stream;
            bool stop_extracting = false;

            // Common section headings that often mark the end of main content
            const std::set<std::string> stop_headings = {
                "see also", "references", "external links", "further reading",
                "bibliography", "notes", "sources", "gallery", "awards",
                "filmography", "discography", "works", "publications"
            };

            // Function to recursively extract content until a stop heading is found
            std::function<void(GumboNode*)> extractContent = [&](GumboNode* node) {
                if (stop_extracting || !node) return;
                if (node->type == GUMBO_NODE_ELEMENT) {

                    // Check if it's a heading that signals the end
                    if (isHeading(node)) {
                        std::string heading_text = getTextContent(node);
                        heading_text = unescapeHtml(heading_text);
                        heading_text = trim(heading_text);
                        // Convert to lowercase for comparison
                        std::transform(heading_text.begin(), heading_text.end(), heading_text.begin(), ::tolower);
                        
                        if (stop_headings.find(heading_text) != stop_headings.end()) {
                            stop_extracting = true;
                            return; // Stop processing this branch
                        }
                    }

                    // If it's a paragraph, list item, or div/table cell (for lists/sections), extract its text
                    GumboTag tag = node->v.element.tag;
                    if (tag == GUMBO_TAG_P || tag == GUMBO_TAG_LI ||
                        tag == GUMBO_TAG_TD || tag == GUMBO_TAG_DIV) {
                        
                        // Avoid extracting text from certain known non-content divs if possible
                        // This is a bit tricky without specific class checks, but we can try
                        // For now, we rely on the stop_heading logic to prevent going too deep
                        
                        std::string element_text = getTextContent(node);
                        element_text = unescapeHtml(element_text);
                        element_text = trim(element_text);
                        
                        if (!element_text.empty()) {
                            // Add a newline before appending new content (except the very first piece)
                            if (content_stream.tellp() > 0) {
                                content_stream << "\n"; // Single newline for structure
                            }
                            content_stream << element_text;
                        }
                    }

                    // Recursively process children unless we've hit a stop condition
                    if (!stop_extracting) {
                        GumboVector* children = &node->v.element.children;
                        for (unsigned int i = 0; i < children->length; ++i) {
                            if (stop_extracting) break;
                            extractContent(static_cast<GumboNode*>(children->data[i]));
                        }
                    }
                }
            };

            extractContent(content_node);
            data.text_content = content_stream.str();
        }
    }
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
}

// Extractor for Wikipedia categories
void extractWikipediaCategories(const std::string& html, ProcessedData& data) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::set<std::string> unique_categories;
    
    if (output && output->root) {
        // Look for category links, often in a div with id 'mw-normal-catlinks'
        std::function<void(GumboNode*)> findCategories = [&](GumboNode* node) {
            if (node->type != GUMBO_NODE_ELEMENT) return;

            // Check if it's a link to a Category page
            if (node->v.element.tag == GUMBO_TAG_A) {
                const char* href = getAttributeValue(node, "href");
                const char* title_attr = getAttributeValue(node, "title");
                
                if (href && title_attr &&
                    std::string(href).find("/wiki/Category:") != std::string::npos &&
                    std::string(title_attr).find("Category:") == 0) {
                    
                    std::string category = std::string(title_attr).substr(9); // Remove "Category:"
                    category = unescapeHtml(category);
                    category = trim(category);
                    if (!category.empty()) {
                        unique_categories.insert(category);
                    }
                }
            }

            // Recursively process children
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                findCategories(static_cast<GumboNode*>(children->data[i]));
            }
        };

        findCategories(output->root);
    }
    
    // Convert set to vector for storage
    data.keywords = std::vector<std::string>(unique_categories.begin(), unique_categories.end());
    
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
}

// Extractor for internal Wikipedia links
void extractWikipediaInternalLinks(const std::string& html, ProcessedData& data) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::set<std::string> unique_links;
    
    if (output && output->root) {
        // Find the main content div first
        std::function<GumboNode*(GumboNode*)> findContentText = [&](GumboNode* node) -> GumboNode* {
            if (node->type != GUMBO_NODE_ELEMENT) return nullptr;
            const char* id = getAttributeValue(node, "id");
            if (id && std::string(id) == "mw-content-text") {
                return node;
            }
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* found = findContentText(static_cast<GumboNode*>(children->data[i]));
                if (found) return found;
            }
            return nullptr;
        };

        GumboNode* content_node = findContentText(output->root);
        if (content_node) {
            // Function to recursively find internal links
            std::function<void(GumboNode*)> findInternalLinks = [&](GumboNode* node) {
                if (node->type != GUMBO_NODE_ELEMENT) return;

                // Check if it's a link that looks like an internal Wikipedia link
                if (node->v.element.tag == GUMBO_TAG_A) {
                    const char* href = getAttributeValue(node, "href");
                    const char* title_attr = getAttributeValue(node, "title");
                    
                    // Basic check for internal links (not starting with http, containing /wiki/, not a special page)
                    if (href && title_attr &&
                        std::string(href).find("http") != 0 &&
                        std::string(href).find("/wiki/") != std::string::npos &&
                        std::string(href).find(":") == std::string::npos) { // Avoid special pages like File:, Category:
                        
                        std::string full_link = "https://en.wikipedia.org" + std::string(href);
                        unique_links.insert(full_link);
                    }
                }

                // Recursively process children
                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                    findInternalLinks(static_cast<GumboNode*>(children->data[i]));
                }
            };

            findInternalLinks(content_node);
        }
    }
    
    // Convert set to vector for storage
    data.links = std::vector<std::string>(unique_links.begin(), unique_links.end());
    
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
}

// Extractor for images
void extractWikipediaImages(const std::string& html, ProcessedData& data) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::set<std::string> unique_images;
    
    if (output && output->root) {
        // Find the main content div first
        std::function<GumboNode*(GumboNode*)> findContentText = [&](GumboNode* node) -> GumboNode* {
            if (node->type != GUMBO_NODE_ELEMENT) return nullptr;
            const char* id = getAttributeValue(node, "id");
            if (id && std::string(id) == "mw-content-text") {
                return node;
            }
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* found = findContentText(static_cast<GumboNode*>(children->data[i]));
                if (found) return found;
            }
            return nullptr;
        };

        GumboNode* content_node = findContentText(output->root);
        if (content_node) {
            // Function to recursively find images
            std::function<void(GumboNode*)> findImages = [&](GumboNode* node) {
                if (node->type != GUMBO_NODE_ELEMENT) return;

                // Check if it's an image tag
                if (node->v.element.tag == GUMBO_TAG_IMG) {
                    const char* src = getAttributeValue(node, "src");
                    // Look for thumbnail images (common class name)
                    const char* class_attr = getAttributeValue(node, "class");
                    
                    if (src && class_attr && std::string(class_attr).find("thumbimage") != std::string::npos) {
                        std::string img_src = src;
                        // Add protocol if missing
                        if (img_src.substr(0, 2) == "//") {
                            img_src = "https:" + img_src;
                        }
                        unique_images.insert(img_src);
                    }
                }

                // Recursively process children
                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                    findImages(static_cast<GumboNode*>(children->data[i]));
                }
            };

            findImages(content_node);
        }
    }
    
    // Convert set to vector for storage
    data.images = std::vector<std::string>(unique_images.begin(), unique_images.end());
    
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
}

// Extractor for infobox data
void extractWikipediaInfobox(const std::string& html, ProcessedData& data) {
    GumboOutput* output = gumbo_parse(html.c_str());
    
    if (output && output->root) {
        // Find the infobox table (usually has class containing 'infobox')
        std::function<GumboNode*(GumboNode*)> findInfobox = [&](GumboNode* node) -> GumboNode* {
            if (node->type != GUMBO_NODE_ELEMENT) return nullptr;
            
            const char* class_attr = getAttributeValue(node, "class");
            if (class_attr && std::string(class_attr).find("infobox") != std::string::npos) {
                return node;
            }
            
            // Recursively search children
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* found = findInfobox(static_cast<GumboNode*>(children->data[i]));
                if (found) return found;
            }
            return nullptr;
        };

        GumboNode* infobox_node = findInfobox(output->root);
        if (infobox_node) {
            // Function to find key-value pairs in infobox rows
            std::function<void(GumboNode*)> extractInfoboxData = [&](GumboNode* node) {
                if (node->type != GUMBO_NODE_ELEMENT) return;

                // Look for table rows
                if (node->v.element.tag == GUMBO_TAG_TR) {
                    GumboNode* header_node = nullptr;
                    GumboNode* data_node = nullptr;
                    
                    // Find th and td children
                    GumboVector* children = &node->v.element.children;
                    for (unsigned int i = 0; i < children->length; ++i) {
                        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
                        if (child->type == GUMBO_NODE_ELEMENT) {
                            if (child->v.element.tag == GUMBO_TAG_TH) {
                                header_node = child;
                            } else if (child->v.element.tag == GUMBO_TAG_TD) {
                                data_node = child;
                            }
                        }
                    }
                    
                    // If we found both a header and data cell
                    if (header_node && data_node) {
                        std::string header_text = getTextContent(header_node);
                        header_text = unescapeHtml(header_text);
                        header_text = trim(header_text);
                        
                        std::string data_text = getTextContent(data_node);
                        data_text = unescapeHtml(data_text);
                        data_text = trim(data_text);
                        
                        if (!header_text.empty() && !data_text.empty()) {
                            // Prefix infobox keys to distinguish them in metadata
                            data.metadata["infobox_" + header_text] = data_text;
                        }
                    }
                }

                // Recursively process children
                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                    extractInfoboxData(static_cast<GumboNode*>(children->data[i]));
                }
            };

            extractInfoboxData(infobox_node);
        }
    }
    
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
}


// This is the function that will be called to register the plugin
extern "C" void registerPlugin(ProcessorRegistry& registry) {
    std::cout << "Registering Gumbo-based Wikipedia plugin..." << std::endl;
    
    auto processor = std::make_unique<PluginProcessor>("wikipedia");
    processor->addExtractor(extractWikipediaTitle);
    processor->addExtractor(extractWikipediaContent);
    processor->addExtractor(extractWikipediaCategories);
    processor->addExtractor(extractWikipediaInternalLinks);
    processor->addExtractor(extractWikipediaImages);
    processor->addExtractor(extractWikipediaInfobox);
    
    registry.registerProcessor("wikipedia", std::move(processor));
}

// Optional plugin information functions
extern "C" const char* getPluginName() {
    return "Gumbo-based Wikipedia Processor Plugin";
}

extern "C" const char* getPluginVersion() {
    return "1.2.0";
}

extern "C" const char* getPluginDescription() {
    return "A robust plugin for processing Wikipedia pages using Gumbo Parser, extracting title, content, categories, links, images, and infobox data.";
}
