#include <iostream>

#include "card.h"
#include "exchange_card.h"
#include "player.h"
#include "big_boss_card.h"

using namespace std;

int main() {

    auto card = ExchangeCard("exchange you", 100, 200);
    ExchangeCard card2 = ExchangeCard("exchange card", 100, 300);
    BigBossCard card3 = BigBossCard("name3", 100, 300);
    Card card4 = Card("name4", 200, 400);
    Card card5 = Card("name5", 300, 600);
    Card card6 = Card("name6", 400, 500);
    
    Player::CardContainerType deck; 
    deck.reserve(9); 
    {
        auto quick_emplace_back = [&](auto &&a) {
            deck.emplace_back(make_unique<std::decay_t<decltype(a)>>(a)); 
        }; 
        auto &&qeb = quick_emplace_back; 
        qeb(card); 
        qeb(card2); 
        qeb(card3); 
        qeb(card4); 
        qeb(card5); 
        qeb(card6); 
        qeb(card4); 
        qeb(card4); 
        qeb(card4); 
    }

    Player::CardContainerType opponent_deck; 
    opponent_deck.reserve(7); 
    {
        auto quick_emplace_back = [&](auto &&a) {
            opponent_deck.emplace_back(make_unique<std::decay_t<decltype(a)>>(a)); 
        };  
        quick_emplace_back(card4); 
        quick_emplace_back(card4); 
        quick_emplace_back(card); 
        quick_emplace_back(card2); 
        quick_emplace_back(card3); 
        quick_emplace_back(card4); 
        quick_emplace_back(card5); 
    }
    
    Player player = Player(std::move(deck), "John");
    Player opponent = Player(std::move(opponent_deck), "Alice");
    
    card.effect(*player.hand.at(0), player, opponent);
    card3.effect(card4, player, opponent);

    cout << opponent.deck.size() << endl;
    cout << player.deck.size() << endl;
    
    player.displayHand();
}