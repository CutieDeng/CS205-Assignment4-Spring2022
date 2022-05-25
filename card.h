#pragma once

#include <iostream> 
#include <string> 
#include <memory> 

/** 
 * Declare the class Player. 
 */ 
class Player;

/** 
 * Define the Card class Information. 
 */ 
class Card {

    friend class Player; 

    /** 
     * Why before define the public access for the data members? 
     * 
     * Boring, of course. 
     */ 
    public: 

    /** 
     * The name of the card. 
     * 
     * Rejects an empty name for safety and convenient. 
     * 
     * Please checks the name valid or not? No blank character! 
     */ 
    std::string name;

    // Attack value of card. 
    int attack;
    // Defense value of card. 
    int defense;

    public: 

    Card(std::string name, int attack, int defense);

    /** 
     * Give a convenient static method to get the pointer. 
     */ 
    static std::unique_ptr<Card> New(std::string name, int attack, int defense) {
        return std::make_unique<Card>(std::move(name), attack, defense); 
    }

    /**
     * @brief power equals to attack - oppenent card's
     *   defense / 2, and will not get a minus value
     * 
     * @param opponentCard opponent card
     * @return double power
     * 
     * Card1: card1 100 200
     * Card2: card2 100 50
     * Card1.power(Card2) => 75
     * Card2.power(Card1) => 0
     */
    double power(Card const &opponentCard);

    /**
     * @brief different card has different effect.
     * 
     * A card has no effect by default
     * 
     * @param oppenentCard
     * @param player
     * @param opponent
     */
    virtual void effect(Card &oppenentCard, Player &player, Player &opponent);

    /**
     * @brief format card output
     * format: <name> <attack> <defense>
     * @param os std::ostream
     * @param card card
     * @return std::ostream&
     * example:
     * name 100 200
     */
    friend std::ostream &operator << (std::ostream &os, Card const &card);

    /** 
     * You must define a virtual deconstructor for avoiding warnings of compilers. 
     */ 
    virtual ~Card(); 
};
