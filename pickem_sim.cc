#include "pickem_sim.h"

void play(Swiss &bracket, uint8_t* matchups)
{
    uint8_t* new_matchups = nullptr;

    // if next matchup is zero, round is done
    if(!matchups[0])
    {
        //bracket.print_standings();
        // if round is 5, we're done!
        if(bracket.round >= 2)
        {
            return;
        }

        // else, advance round
        bracket.round++;

        // allocate a new array and calculate new matchups
        new_matchups = new uint8_t(9);
        matchups = new_matchups;
        bracket.get_matchups(matchups);
    }

    // grab the probability of team0 winning this matchup
    double p_team0 = 0.5f;

    // make a copy of the current bracket
    Swiss new_bracket(bracket);

    // play the scenario where team0 wins on the original bracket
    bracket.play_match(TEAM0(matchups[0]), TEAM1(matchups[0]), p_team0);
    play(bracket, &matchups[1]);

    // play the scenario where team1 wins on the copy
    new_bracket.play_match(TEAM1(matchups[0]), TEAM0(matchups[0]), 1-p_team0);
    play(new_bracket, &matchups[1]);

    // free up allocated memory
    delete new_matchups;
}

int main()
{
    Swiss bracket;
    uint8_t matchups[9] = {};

    bracket.get_matchups(matchups);
    play(bracket, matchups);

    std::cout << "Done" << std::endl;
    bracket.print_standings();

    return 0;
}




