#include <algorithm> 
#include <any> 
#include <cassert> 
#include <cstdint> 
#include <filesystem> 
#include <fstream> 
#include <iostream> 
#include <map> 
#include <memory> 
#include <sstream> 
#include <string> 
#include <tuple> 
#include <type_traits> 
#include <variant> 

#include "player.h" 
#include "card.h" 
#include "big_boss_card.h" 
#include "exchange_card.h" 

using std::monostate; 

using i32 = int32_t; 
using i64 = int64_t; 
using u32 = uint32_t; 
using u64 = uint64_t; 

constexpr auto INFO_OUTPUT = false; 

namespace TypeTrait {

    namespace Helper {
        template <typename T> 
        std::enable_if_t<std::is_reference_v<decltype(std::declval<std::ostream>() << std::declval<T>())> && 
            std::is_same_v<std::decay_t<decltype(std::declval<std::ostream>() << std::declval<T>())>, std::ostream>, std::true_type> display(T const &t) {
            std::cout << t; 
            return {}; 
        }
        std::false_type display(...); 
    }

    template <typename T> 
    bool display(T const &t) {
        constexpr bool can_display = decltype(TypeTrait::Helper::display(t))::value;  
        if constexpr (can_display) {
            TypeTrait::Helper::display(t); 
            std::cout << '\n'; 
        } else {
            std::clog << "[ERROR] Cannot display the object! Please finish the operator<< function for this object! \n"; 
        }
        return can_display; 
    }
}

void display(std::any const &self) {
    // std::cout << decltype(TypeTrait::display(self))::value << '\n'; 
    if (auto int_ptr = std::any_cast<int>(&self); int_ptr) {
        TypeTrait::display(*int_ptr); 
    } else if (auto str_ptr = std::any_cast<std::string>(&self); str_ptr) {
        TypeTrait::display(*str_ptr); 
    } else if (auto cstr_ptr = std::any_cast<char const *>(&self); cstr_ptr) {
        TypeTrait::display(*cstr_ptr); 
    } else if (auto card_ptr = std::any_cast<Card>(&self); card_ptr) {
        TypeTrait::display(*card_ptr); 
    } else if (auto player_ptr = std::any_cast<Player>(&self); player_ptr) {
        TypeTrait::display(*player_ptr); 
    } else if (auto bool_ptr = std::any_cast<bool>(&self); bool_ptr) {
        TypeTrait::display(*bool_ptr); 
    } else if (auto double_ptr = std::any_cast<double>(&self); double_ptr) {
        TypeTrait::display(*double_ptr); 
    } else if (auto void_ptr = std::any_cast<std::monostate>(&self); void_ptr) {
        std::cout << "void\n"; 
    } else {
        std::clog << "[ERROR] Cannot display the unknown type information! \n"; 
    }
}


std::any generate_from_stream (std::istream &is, std::string_view file_name) {
    std::string this_line; 

    std::vector<std::pair<std::string, std::vector<std::any>>> command_collection; 

    using std::literals::operator""s, std::make_any, std::any_cast, std::make_unique; 

    struct ArgumentSizeNotEnough {}; 

    struct ArgumentOperatorError {
        i32 index = 0; 
    }; 
    
    // static std::map<std::string, std::unique_ptr<Card>> origin_card_collection; 
    static std::map<std::string, std::any> origin_card_collection; 
    static std::map<std::string, std::string> command_alias; 

    static std::map<std::string, std::function<std::any (std::string const &, std::vector<std::any> &)>> dealing_map = 
    {
        {"INCLUDE", [](auto &&fn, auto &input) {
            if (input.empty()) {
                throw ArgumentSizeNotEnough{}; 
            }
            auto &&file_name = input.at(0); 
            if (auto string_p = any_cast<std::string>(&file_name); string_p) {
                std::filesystem::path file_path (*string_p); 
                std::ifstream fs (file_path); 
                if (!fs.is_open()) {
                    std::clog << "[WARNING] INCLUDE command attempt to open the file '" << *string_p << "' but fails! \n"; 
                    return make_any<std::monostate>(); 
                }
                return generate_from_stream(fs, *string_p); 
            } else {
                throw ArgumentOperatorError{}; 
            }
        }}, 
        {"NOP", [](auto &&, auto &) {
            return std::monostate{}; 
        }}, 
        {"SILENT", [](auto &&, auto &argu) {
            if (argu.empty()) 
                throw ArgumentSizeNotEnough{}; 
            if constexpr (INFO_OUTPUT)
                std::clog << "[INFO] Silent one output command. \n"; 
            return std::monostate{}; 
        }}, 
        {
            "COMMANDALIAS", [](auto &&, auto &argu) {
                if (argu.size() < 2)    
                    throw ArgumentSizeNotEnough{}; 
                std::string origin, new_command_name; 
                if (auto str_ptr = any_cast<std::string>(&argu.at(0)); str_ptr) 
                    origin = std::move(*str_ptr); 
                else {
                    throw ArgumentOperatorError{}; 
                } 
                if (auto str_ptr = any_cast<std::string>(&argu.at(1)); str_ptr) 
                    new_command_name = std::move(*str_ptr); 
                else {
                    throw ArgumentOperatorError{}; 
                } 
                for (auto &&o: origin) {
                    if (o >= 'a' && o <= 'z') 
                        o += 'A' - 'a'; 
                }
                for (auto &&o: new_command_name) {
                    if (o >= 'a' && o <= 'z') 
                        o += 'A' - 'a'; 
                }
                if (auto str = dealing_map.find(origin); str != dealing_map.end()) {
                    // We can do the simple command alias! 
                    command_alias[new_command_name] = std::move(origin); 
                } else {
                    throw ArgumentOperatorError{}; 
                }
                return make_any<monostate>(); 
            }
        }, 
        {
            "CARD", 
            [](auto &&, auto &argu) {
                if constexpr (INFO_OUTPUT) 
                    std::clog << "[INFO] Invoke CARD construct with argu.size = " << argu.size() << '\n'; 
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                if (argu.size() == 1) {
                    if (auto name_p = any_cast<std::string>(&argu.at(0)); name_p) {
                        if (auto fetch = origin_card_collection.find(*name_p); fetch != origin_card_collection.end()) {
                            // auto &&card = *fetch->second;
                            // // Need a dynamic analysis! 
                            // if (auto big_boss_card_p = dynamic_cast<BigBossCard*>(&card); big_boss_card_p) {
                            //     return make_any<BigBossCard>(*big_boss_card_p); 
                            // } else if (auto exchange_card_p = dynamic_cast<ExchangeCard*>(&card); exchange_card_p) {
                            //     return make_any<ExchangeCard>(*exchange_card_p); 
                            // } else {
                            //     return make_any<Card>(card); 
                            // }
                            
                            /** 
                             * It's to complex to use unique pointer.  
                             */ 
                             return fetch->second; 
                        } 
                    } else {
                        throw ArgumentOperatorError{}; 
                    }
                }

                if (argu.size() < 3) 
                    throw ArgumentSizeNotEnough{}; 
                
                std::string card_name; 
                if (auto str_p = any_cast<std::string>(&argu.at(0)); str_p) {
                    card_name = std::move(*str_p); 
                }
                else 
                    throw ArgumentOperatorError{}; 

                double attack; 
                if (auto d_p = any_cast<double>(&argu.at(1)); d_p) {
                    attack = *d_p; 
                }
                else 
                    throw ArgumentOperatorError{}; 

                double defence; 
                if (auto d_p = any_cast<double>(&argu.at(2)); d_p) {
                    defence = *d_p; 
                }
                else 
                    throw ArgumentOperatorError{}; 

                Card c (card_name, attack, defence); 

                origin_card_collection[card_name] = c; 
                return make_any<Card>(std::move(c)); 
            }
        }, 
        {
            "DISPLAY", 
            [](auto &&, auto &argu) {
                if constexpr (INFO_OUTPUT)
                    std::clog << "[INFO] Invoke Display! \n"; 
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                assert (argu.size() == 1); 
                if (auto str_ptr = any_cast<std::string>(&argu.at(0)); str_ptr) {
                    // attempt to match it! 
                    if (auto card_ptr = origin_card_collection.find(*str_ptr); card_ptr != origin_card_collection.end()) {
                        TypeTrait::display(any_cast<Card>(card_ptr->second)); 
                        // display(*card_ptr); 
                    } else if (auto player_ptr = 1; false) {
                        
                    } else {
                        TypeTrait::display(*str_ptr); 
                    }
                } else {
                    display(argu.at(0)); 
                }
                return make_any<std::monostate>(); 
            }
        }, 
        {
            "TRUE", 
            [](auto &&, auto &) {
                return true; 
            }
        }, 
        {
            "FALSE", 
            [](auto &&, auto &) {
                return false; 
            }
        }, 
    }; 

    while (1) {

        getline(is, this_line); 
        if (!is) 
            break; 
        
        std::stringstream this_line_token_stream (std::move(this_line)); 
        std::string token; 

        i64 line_id = -1;

        while (1) {
            ++line_id; 
            this_line_token_stream >> token; 

            if (!this_line_token_stream) 
                break; 

            if (token.empty()) 
                continue; 
            
            // The annotation prefix evaluation. 
            if (token.at(0) == '#') 
                break;
            
            {
                // std::clog << "Find token (" << token << "). \n"; 
                start_deal: 
                auto it = dealing_map.find(token); 
                if (it != dealing_map.end()) {
                    command_collection.push_back({std::move(token), {}}); 
                } else {
                    constexpr auto is_upper_totally = [](auto &s) -> bool {
                        for (auto &&i: s) {
                            if (i >= 'A' && i <= 'Z') {} 
                            else return false; 
                        }
                        return true; 
                    }; 
                    if (is_upper_totally(token)) {
                        if (auto al_ptr = command_alias.find(token); al_ptr != command_alias.end()) {
                            token = al_ptr->second; 
                            goto start_deal; 
                        } else {
                            std::clog << "[WARNING] You input a command-style string, but it actually is not a COMMAND! \n"; 
                        }
                    }
                    // Check it as a double, or string! 
                    if (!command_collection.empty()) {
                        try {
                            auto d = std::stod(token); 
                            command_collection.back().second.push_back(d); 
                        } catch (std::invalid_argument &) {
                            command_collection.back().second.push_back(std::move(token)); 
                        }
                    } else {
                        std::clog << "Empty Command with argument '" << token << "'\n"; 
                    }
                }
                try {
                    repeatedly: 
                    if (command_collection.empty()) 
                        continue; 
                    std::any result; 
                    result = dealing_map.find(command_collection.back().first)->second(token, command_collection.back().second); 
                    command_collection.pop_back(); 
                    if (auto void_ptr = any_cast<std::monostate>(&result); !void_ptr) {
                        if (command_collection.empty()) {
                            std::clog << "Get a value but Drop! The information of it: \n"; 
                            ::display(result); 
                        } else {
                            command_collection.back().second.push_back(std::move(result)); 
                        }
                    }
                    goto repeatedly; 
                } catch (ArgumentSizeNotEnough &) {
                    // Skip it. 
                } catch (ArgumentOperatorError &) {
                    // Give a feed back for the false. 
                    std::clog << "Attempt the command '" << command_collection.back().first << "' but fails with the confusing programming arguments! \nThe related command is nearly "
                        << file_name << ":" << line_id << '\n'; 
                    command_collection.pop_back(); 
                    if (!command_collection.empty())
                        command_collection.back().second.push_back(std::monostate{}); 
                } 
            }
        }
    }

    return monostate{}; 
}

int main() {
    std::cout.setf(std::ios::boolalpha); 
    std::clog.setf(std::ios::boolalpha); 

    std::ifstream f (std::filesystem::path{"script.txt"}); 
    if (f.is_open()) 
        generate_from_stream(f, "script.txt"); 
}