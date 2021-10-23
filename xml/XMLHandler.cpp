/* Created by LordRibblesdale on 01/08/2021.
 * MIT License
 */

#include "XMLHandler.hpp"

#include <iostream>
#include <fstream>
#include <pugixml.hpp>

// https://github.com/zeux/pugixml
void XMLHandler::loadXMLSettings(const std::string &path, Settings& settings) {
    std::ifstream file(path);

    if (file.is_open()) {
        pugi::xml_document xmlDocument;
        pugi::xml_parse_result result = xmlDocument.load_file(path.c_str());

        if (!result) {
            std::cout << "<ERROR> result loadXMLSettings: XML file not loaded.";
            return;
        }


    } else {
        // TODO check if terminal prints them out
        std::cout << "<ERROR> loadXMLSettings: file not opened." << std::endl;
    }

    file.close();
}
