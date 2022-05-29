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
            std::cout << "[ERROR] Cannot display the object! Please finish the operator<< function for this object! \n"; 
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
        std::cout << "[ERROR] Cannot display the unknown type information! \n"; 
    }
}

std::unique_ptr<Card> make_from_any(std::any const &a) {
    if (auto boss_ptr = std::any_cast<BigBossCard>(&a); boss_ptr) {
        return std::make_unique<BigBossCard>(*boss_ptr); 
    } else if (auto exchange_ptr = std::any_cast<ExchangeCard>(&a); exchange_ptr) {
        return std::make_unique<ExchangeCard>(*exchange_ptr); 
    } else if (auto card_ptr = std::any_cast<Card>(&a); card_ptr) {
        return std::make_unique<Card>(*card_ptr); 
    } else return nullptr; 
}

std::unique_ptr<Card> clone_unique_ptr_as_card(std::unique_ptr<Card> const &a) {
    auto p = a.get(); 
    if (auto boss_ptr = dynamic_cast<BigBossCard*>(p); boss_ptr) {
        return std::make_unique<BigBossCard>(*boss_ptr); 
    } else if (auto exchange_ptr = dynamic_cast<ExchangeCard*>(p); exchange_ptr) {
        return std::make_unique<ExchangeCard>(*exchange_ptr); 
    } else return std::make_unique<Card>(*p); 
}

std::any clone_unique_ptr_as_any (std::unique_ptr<Card> const &a) {
    auto p = a.get(); 
    if (auto boss_ptr = dynamic_cast<BigBossCard*>(p); boss_ptr) {
        return std::make_any<BigBossCard>(*boss_ptr); 
    } else if (auto exchange_ptr = dynamic_cast<ExchangeCard*>(p); exchange_ptr) {
        return std::make_any<ExchangeCard>(*exchange_ptr); 
    } else return std::make_any<Card>(*p); 
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
    static std::map<std::string, Player> players; 

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
                    std::cout << "[WARNING] INCLUDE command attempt to open the file '" << *string_p << "' but fails! \n"; 
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
                std::cout << "[INFO] Silent one output command. \n"; 
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
            "BIGBOSSCARD", 
            [](auto &&, auto &argu) {
                if constexpr (INFO_OUTPUT) 
                    std::cout << "[INFO] Invoke BIGBOSSCARD construct with argu.size = " << argu.size() << '\n'; 

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

                BigBossCard c (card_name, attack, defence); 

                origin_card_collection[card_name] = c; 
                return make_any<BigBossCard>(std::move(c)); 
            }
        }, 
        {
            "DECKSIZE", 
            [](auto &&, auto &argu) {
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                if (auto str_p = any_cast<std::string>(&argu.at(0)); str_p) {
                    if (auto player_ptr = players.find(*str_p); player_ptr != players.end()) {
                        argu.at(0) = &player_ptr->second; 
                    }
                }
                try {
                    auto player = any_cast<Player*>(argu.at(0)); 
                    return make_any<double>(player->deck.size()); 
                } catch (std::bad_any_cast &) {
                    throw ArgumentOperatorError{};
                }
            }
        }, 
        {
            "HANDSIZE", 
            [](auto &&, auto &argu) {
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                if (auto str_p = any_cast<std::string>(&argu.at(0)); str_p) {
                    if (auto player_ptr = players.find(*str_p); player_ptr != players.end()) {
                        argu.at(0) = &player_ptr->second; 
                    }
                }
                try {
                    auto player = any_cast<Player*>(argu.at(0)); 
                    return make_any<double>(player->hand.size()); 
                } catch (std::bad_any_cast &) {
                    throw ArgumentOperatorError{};
                }
            }
        }, 
        {
            "EXCHANGECARD", 
            [](auto &&, auto &argu) {
                if constexpr (INFO_OUTPUT) 
                    std::cout << "[INFO] Invoke EXCHANGECARD construct with argu.size = " << argu.size() << '\n'; 

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

                ExchangeCard c (card_name, attack, defence); 

                origin_card_collection[card_name] = c; 
                return make_any<ExchangeCard>(std::move(c)); 
            }
        }, 
        {
            "CARD", 
            [](auto &&, auto &argu) {
                if constexpr (INFO_OUTPUT) 
                    std::cout << "[INFO] Invoke CARD construct with argu.size = " << argu.size() << '\n'; 
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
                    std::cout << "[INFO] Invoke Display! \n"; 
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                assert (argu.size() == 1); 
                if (auto str_ptr = any_cast<std::string>(&argu.at(0)); str_ptr) {
                    // attempt to match it! 
                    auto card_ptr = origin_card_collection.find(*str_ptr);
                    auto player_ptr = players.find(*str_ptr); 
                    if (card_ptr != origin_card_collection.end() && player_ptr != players.end()) {
                        std::cout << "[WARNING] ambiguous name for objects to display! \n"; 
                        return make_any<monostate>(); 
                    } else if (card_ptr != origin_card_collection.end()) {
                        TypeTrait::display(any_cast<Card>(card_ptr->second)); 
                    } else if (player_ptr != players.end()) {
                        player_ptr->second.displayHand(); 
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
        {
            "POWER", 
            [](auto &&, auto &argu) {
                if (argu.size() < 2) 
                    throw ArgumentSizeNotEnough{}; 
                auto &card_obj = (argu.at(0)); 
                if (auto str_ptr = any_cast<std::string>(&card_obj); str_ptr) {
                    if (auto actual_card = origin_card_collection.find(*str_ptr); actual_card != origin_card_collection.end()) {
                        card_obj = actual_card->second; 
                    } else {
                        throw ArgumentOperatorError{}; 
                    }
                }
                Card *tmp_obj {any_cast<Card>(&card_obj)}; 
                auto &card_obj2 = argu.at(1); 
                if (auto str_ptr = any_cast<std::string>(&card_obj2); str_ptr) {
                    if (auto actual_card = origin_card_collection.find(*str_ptr); actual_card != origin_card_collection.end()) {
                        card_obj2 = actual_card->second; 
                    } else {
                        throw ArgumentOperatorError{}; 
                    }
                }
                Card *tmp_obj2 {any_cast<Card>(&card_obj2)}; 
                if (tmp_obj && tmp_obj2) 
                    return tmp_obj->power(*tmp_obj2); 
                else 
                    throw ArgumentOperatorError{}; 
            }
        }, 
        {
            "EFFECT", 
            [] (auto &&, auto &argu) {
                if (argu.size() < 4) 
                    throw ArgumentSizeNotEnough{}; 

                if (auto str_p = any_cast<std::string>(&argu.at(1)); str_p) {
                    if (auto player_ptr = players.find(*str_p); player_ptr != players.end()) {
                        argu.at(1) = &player_ptr->second; 
                    }
                }
                if (auto str_p = any_cast<std::string>(&argu.at(3)); str_p) {
                    if (auto player_ptr = players.find(*str_p); player_ptr != players.end()) {
                        argu.at(3) = &player_ptr->second; 
                    }
                }

                if (auto str_p = any_cast<std::string>(&argu.at(0)); str_p) {
                    if (auto card_p = origin_card_collection.find(*str_p); card_p != origin_card_collection.end()) {
                        argu.at(0) = card_p->second; 
                    }
                }
                if (auto str_p = any_cast<std::string>(&argu.at(2)); str_p) {
                    if (auto card_p = origin_card_collection.find(*str_p); card_p != origin_card_collection.end()) {
                        argu.at(2) = card_p->second; 
                    }
                }   
                try {
                    assert (any_cast<Card>(&argu.at(2))); 
                    any_cast<Card>(argu.at(0)).effect(*any_cast<Card>(&argu.at(2)), *any_cast<Player*>(argu.at(1)), 
                        *any_cast<Player*>(argu.at(3))); 
                } catch (std::bad_any_cast &) {
                    throw ArgumentOperatorError{}; 
                }             
                return make_any<monostate>(); 
            }
        }, 
        {
            "DECK", 
            [](auto &&, auto &argu) {
                if (argu.empty() || argu.back().type() != typeid(std::string) || any_cast<std::string>(argu.back()) != "end") 
                    throw ArgumentSizeNotEnough{}; 
                if constexpr (INFO_OUTPUT)
                    std::cout << "[INFO] Invoke DECK! \n"; 
                std::vector<std::any> deck; 
                for (size_t i{}; i + 1 < argu.size(); ++i) {
                    if (auto str_p = any_cast<std::string>(&argu.at(i)); str_p) {
                        if (auto card_p = origin_card_collection.find(*str_p); card_p != origin_card_collection.end()) {
                            argu.at(i) = card_p->second; 
                        }  
                    }
                    if (auto card_p = any_cast<Card>(&argu.at(i)); card_p) {
                        deck.push_back(std::move(argu.at(i))); 
                    } else {
                        std::cout << "[WARNING] The obj cannot be added in the deck: \n"; 
                        display(argu.at(i)); 
                        std::cout << "------\n"; 
                    }
                }
                return deck; 
            }, 
        }, 
        {
            "PLAYER", 
            [](auto &&, auto &argu) -> std::any {
                if (argu.size() == 1) {
                    if (auto str = any_cast<std::string>(&argu.at(0)); str) {
                        if (auto player_p = players.find(*str); player_p != players.end()) {
                            return make_any<Player*>(&player_p->second); 
                        }
                    } else {
                        throw ArgumentOperatorError{}; 
                    }
                }
                if (argu.size() < 2) 
                    throw ArgumentSizeNotEnough{}; 
                try {
                    std::string name = any_cast<std::string>(std::move(argu.at(0))); 
                    auto deck = any_cast<std::vector<std::any>>(std::move(argu.at(1))); 
                    using DeckType = decltype(Player::deck); 
                    static_assert (!std::is_reference_v<DeckType>); 
                    if constexpr (std::is_same_v<DeckType::value_type, Card>) {
                        DeckType construct_deck; 
                        for (auto &&i: deck) {
                            if (auto card_cast = any_cast<Card>(&i); card_cast) {
                                construct_deck.emplace_back(*card_cast); 
                            }
                        }
                        auto r = players.emplace(std::make_tuple<std::string, Player>(std::string(name), Player{std::move(construct_deck), name})); 
                        return make_any<Player*>(&r.first->second);
                    } else if constexpr (std::is_same_v<DeckType::value_type, std::unique_ptr<Card>>) {
                        DeckType construct_deck; 
                        for (auto &&i: deck) {
                            auto p = make_from_any(i); 
                            if (p) 
                                construct_deck.push_back(std::move(p)); 
                        }
                        auto r = players.emplace(std::make_tuple<std::string, Player>(std::string(name), Player{std::move(construct_deck), name})); 
                        return make_any<Player*>(&r.first->second);
                    } 
                    std::cout << "[ERROR] Deck Type ERROR!!! \n"; 
                    throw ArgumentOperatorError{}; 
                } catch (std::bad_any_cast &) {
                    throw ArgumentOperatorError{}; 
                }
                return make_any<monostate>(); 
            }
        }, 
        {
            "ASSERTEQ", 
            [](auto &&, auto &argu) {
                if (argu.size() < 2) 
                    throw ArgumentSizeNotEnough{}; 
                try {
                    auto lhs = any_cast<double>(argu.at(0)); 
                    auto rhs = any_cast<double>(argu.at(1)); 
                    lhs -= rhs; 
                    if (lhs < 0) 
                        lhs = -lhs; 
                    if (lhs <= std::numeric_limits<double>::epsilon()) {
                        std::cout << "Assert Pass. \n";    
                    } else {
                        std::cout << "Assert Fail! \n"; 
                    }
                    return make_any<monostate>();
                } catch (std::bad_any_cast &) {
                    throw ArgumentOperatorError{}; 
                }
            }
        }, 
        {
            "DRAW", 
            [] (auto &&, auto &argu) {
                if (argu.empty()) 
                    throw ArgumentSizeNotEnough{}; 
                if (auto str = any_cast<std::string>(&argu.at(0)); str) {
                    if (auto player_p = players.find(*str); player_p != players.end()) {
                        argu.at(0) = &player_p->second; 
                    }
                }
                if (auto play_p = any_cast<Player*>(&argu.at(0)); play_p) {
                    (*play_p)->draw(); 
                    return make_any<monostate>(); 
                } else {
                    throw ArgumentOperatorError{}; 
                }
            }
        }, 
        {
            "PLAY", 
            [] (auto &&, auto &argu) {
                if (argu.size() < 2) 
                    throw ArgumentSizeNotEnough{}; 
                if (auto str = any_cast<std::string>(&argu.at(0)); str) {
                    if (auto player_p = players.find(*str); player_p != players.end()) {
                        argu.at(0) = &player_p->second; 
                    }
                }
                try {
                    auto player = any_cast<Player*>(argu.at(0)); 
                    auto val = any_cast<double>(argu.at(1)); 
                    if ((i64)val != val) {
                        std::cout << "[WARNING] Input a floating value as index! \nJust truncate it as " << (i64)val << '\n'; 
                    }
                    auto result = player->play((i64)val); 
                    if constexpr (std::is_same_v<decltype(result), Card>) {
                        return make_any(result); 
                    } else if (std::is_same_v<decltype(result), std::unique_ptr<Card>>) {
                        return clone_unique_ptr_as_any(result); 
                    } else {
                        std::cout << "[ERROR] Invalid draw result realization! \n"; 
                        throw ArgumentOperatorError{}; 
                    }
                } catch (std::bad_any_cast &) {
                    throw ArgumentOperatorError{}; 
                }
            }
        }
    }; 

    i64 line_id {};

    while (1) {

        ++line_id; 

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
            
            {
                // std::cout << "Find token (" << token << "). \n"; 
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
                            std::cout << "[WARNING] You input a command-style string, but it actually is not a COMMAND! \n"; 
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
                        std::cout << "Empty Command with argument '" << token << "'\n"; 
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
                            std::cout << "Get a value but Drop! The information of it: \n"; 
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
                    std::cout << "Attempt the command '" << command_collection.back().first << "' but fails with the confusing programming arguments! \nThe related command is nearly "
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

    std::ifstream f (std::filesystem::path{"script.txt"}); 
    if (f.is_open()) 
        generate_from_stream(f, "script.txt"); 
}