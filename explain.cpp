#include <any> 
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
    
    static std::map<std::string, std::unique_ptr<Card>> origin_card_collection; 

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
        {
            "CARD", 
            [](auto &&, auto &argu) {
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                if (argu.size() == 1) {
                    if (auto name_p = any_cast<std::string>(&argu.at(0)); name_p) {
                        if (auto fetch = origin_card_collection.find(*name_p); fetch != origin_card_collection.end()) {
                            auto &&card = *fetch->second;
                            // Need a dynamic analysis! 
                            if (auto big_boss_card_p = dynamic_cast<BigBossCard*>(&card); big_boss_card_p) {
                                return make_any<BigBossCard>(*big_boss_card_p); 
                            } else if (auto exchange_card_p = dynamic_cast<ExchangeCard*>(&card); exchange_card_p) {
                                return make_any<ExchangeCard>(*exchange_card_p); 
                            } else {
                                return make_any<Card>(card); 
                            }
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

                origin_card_collection[card_name] = make_unique<Card>(c); 
                return make_any<Card>(std::move(c)); 
            }
        }, 
    }; 

    while (1) {

        getline(is, this_line); 
        if (!is) 
            break; 
        
        std::stringstream this_line_token_stream (std::move(this_line)); 
        std::string token; 

        while (1) {

            this_line_token_stream >> token; 

            if (!this_line_token_stream) 
                break; 

            if (token.empty()) 
                continue; 
            
            // The annotation prefix evaluation. 
            if (token.at(0) == '#') 
                break;
            

        }
    }

    return monostate{}; 
}

int main() {
    std::ifstream f (std::filesystem::path{"hello.txt"}); 
    std::cout.setf(std::ios::boolalpha); 
    std::clog.setf(std::ios::boolalpha); 
    std::cout << f.is_open() << '\n'; 
    display("12"); 
}