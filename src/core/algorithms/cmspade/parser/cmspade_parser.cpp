#include "cmspade_parser.h"

namespace algos::cmspade::parser{
CMSpadeParser::CMSpadeParser(std::filesystem::path filepath) : filepath_(filepath) {
    file_.open(filepath_);
    if (!file_){
        throw std::runtime_error("Error: couldn't find file " + filepath_.string());
    }
}

CMSpadeParser::~CMSpadeParser(){
    if (file_.is_open()){
        file_.close();
    }
}

std::vector<std::unique_ptr<Sequence>>CMSpadeParser::ParseAll() {

    std::vector<std::unique_ptr<Sequence>> sequences;
    SequenceId sequence_id = 0;
    std::unique_ptr<Sequence> sequence;
    
    while (ParseNextSequence(sequence_id, sequence)) {
        if (sequence) {
            sequences.push_back(std::move(sequence));
            sequence_id++;
        }
    }
    return sequences;
}

bool CMSpadeParser::ParseNextSequence(SequenceId sequence_id, 
                        std::unique_ptr<Sequence>& sequence) {
    std::string line;
    
    while (std::getline(file_, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%' || line[0] == '@') {
            continue;
        }
        
        ParseSequenceLine(line, sequence_id, sequence);
        return true;
    }
    
    return false;
}

void CMSpadeParser::ParseSequenceLine(const std::string& line, SequenceId sequence_id, 
                        std::unique_ptr<Sequence>& sequence){
    sequence = std::make_unique<Sequence>(sequence_id);
    auto current_itemset = std::make_unique<Itemset>();

    std::istringstream iss(line);
    std::string token;

    while(iss >> token){
        if (token[0] == '<'){
            continue;
        }

        if (token == "-1"){
            if (!current_itemset->empty()) {
                sequence->AddItemset(std::move(current_itemset));
                current_itemset = std::make_unique<Itemset>();
            }
        }

        else if (token == "-2"){
            if (!current_itemset->empty()) {
                sequence->AddItemset(std::move(current_itemset));
            }

            break;
        }

        else{
            Int item_id = std::stoi(token);
            Item item(item_id);
            current_itemset->AddItem(item);
        }
    }

    if (!current_itemset->empty()) {
        sequence->AddItemset(std::move(current_itemset));
    }
}
} // namespace algos::cmspade::parser