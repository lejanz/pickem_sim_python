#include "pickem_sim.h"

#include <chrono>

#define WL_MAX 2
#define ROUNDS 5
#define MIN_CHANCE 0.01f

uint64_t iteration_counter = 0;
double team_p_wl[N_TEAMS][16] = {0.0f};

double p_time = 0.0f;

void play(Swiss &bracket, uint8_t* matchups)
{
    // iteration_counter++;
    // if (iteration_counter > 1e8)
    // {
    //     return;
    // }

    static uint8_t matchup_stack[100]; // prety sure we only need 29 or something but im scared
    static uint64_t ms_counter = 0;
    uint64_t n_new_matches = 0;

    // if next matchup is zero, round is done
    if(!matchups[0])
    {
        //bracket.print_standings();
        // if round is 5, we're done!
        if(bracket.round >= ROUNDS)
        {

            // auto start = std::chrono::high_resolution_clock::now();
            // add probabilities
            for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
            {
                uint64_t wl = (bracket.team_wl >> INV_TEAM_SHIFT(team_id)) & 0xF;
                team_p_wl[team_id][wl] += bracket.scenario_probability;
            }
            // auto end = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<double, std::milli> duration = end - start;
            // p_time += duration.count();
            // exit
            return;
        }

        // else, advance round
        bracket.round++;

        // add new matchups to the matchup stack
        matchups = &matchup_stack[ms_counter];
        n_new_matches = bracket.get_matchups(matchups);
        ms_counter += n_new_matches;
        // std::cout << "Added matches. Counter: " << ms_counter << std::endl;
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
    ms_counter -= n_new_matches;
}

void print_chances()
{
    for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
    {
        std::cout << team_names[team_id];
        for (uint8_t wl = 0; wl <= 0xF; wl++)
        {
            double p_wl = team_p_wl[team_id][wl];
            if (p_wl < MIN_CHANCE)
            {
                continue;
            }
            uint8_t wins = WINS(wl);
            uint8_t losses = LOSSES(wl);
            // if (!(wins == WL_MAX || losses == WL_MAX))
            // {
            //     continue;
            // }
            std::cout << " " << +wins << +losses << ": " << team_p_wl[team_id][wl];
        }
        std::cout << std::endl;
    }
}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    Swiss bracket;
    uint8_t matchups[9] = {};

    bracket.get_matchups(matchups);
    play(bracket, matchups);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Done. Iterations: " << iteration_counter << " Time: " << duration.count() << "ms" << std::endl;
    std::cout << "Probability time: " << p_time << "ms" << std::endl;
    // bracket.print_standings();

    print_chances();

    for (uint8_t round = 0; round < 5; round++)
    {
        std::cout << "Round " << 1 + round << ": n=" << n_time_ms[round] << ", " << avg_time_ms[round]/n_time_ms[round] 
        << "ms avg, " << avg_time_ms[round] << "ms total" << std::endl;  
    }

    return 0;
}




