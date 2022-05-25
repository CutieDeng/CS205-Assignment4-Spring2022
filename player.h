#pragma once

#include <iostream>
#include <vector>
#include <memory> 
#include <sstream> 

#include "card.h"

class Player {
   public:

    /** 
     * Define a special type alias for the container to store the different cards. 
     */ 
    typedef std::vector<std::unique_ptr<Card>> CardContainerType; 

    /** 
     * The card deck! 
     * 
     * It's stupid for you to give a Card instance in the container. 
     * Please use a unique pointer to avoid it! 
     */ 
    CardContainerType deck;

    // name of player 
    std::string name; 

    /** 
      * The Card Container of your hands!
      * 
      * Players should draw a card from deck to his hand! 
      */ 
    CardContainerType hand;

    Player(CardContainerType &deck, std::string name); 
    /**
     * Draw a card from the top [actually back] of the deck container. 
     */
    void draw();

    /**
     * Return the card out according to the given index. 
     * 
     * Please remove the card in your hand container, of course. 
     * 
     * If find no card in that index, the exception InvalidIndextoFindCard should be given! 
     */
    std::unique_ptr<Card> play(int index);

    /** 
     * The customized exception class. 
     * 
     * The static method can help you to quickly construct an exception with enough message! 
     */ 
    struct InvalidIndextoFindCard : public std::logic_error {
        using std::logic_error::logic_error; 

        template <char const *container_name, typename Container>  
        static InvalidIndextoFindCard find_out_of_index (size_t index, Container const &container) {
            std::string error_message = "Invalid Index to Find the Card: Index("; 
            error_message.reserve(65); 
            error_message += std::to_string(index) ; 
            error_message += ") is out of the card container ["; 
            error_message += container_name; 
            error_message += "] {begin: 0, end: "; 
            error_message += std::to_string(container.size()); 
            error_message += "}. "; 
            return InvalidIndextoFindCard(std::move(error_message)); 
        }

        template <typename A, typename B> 
        static InvalidIndextoFindCard find_out_of_index_in_deck(A &&a, B &&b) {
            return InvalidIndextoFindCard<"deck">(std::forward<A>(a), std::forward<B>(b)); 
        }

        template <typename A, typename B> 
        static InvalidIndextoFindCard find_out_of_index_in_hand(A &&a, B &&b) {
            return InvalidIndextoFindCard<"hand">(std::forward<A>(a), std::forward<B>(b)); 
        }
    }

    /**
     * Display all cards in hand, every card is displayed
     *   in one line
     * 
     * example:
     * card1 10 10
     * card2 20 20
     * card3 20 20
     */
    void displayHand();
};

