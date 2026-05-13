#include "pickem_sim.h"

#include <chrono>

#define WL_MAX 3
#define ROUNDS 5
#define MIN_CHANCE 0.01f

uint64_t iteration_counter = 0;
double matchup_chances[N_TEAMS*N_TEAMS] = {};
double team_p_wl[N_TEAMS][16] = {0.0f};

double p_time = 0.0f;

void complement_matchup_chances()
{
    for (int team_id = 0; team_id < N_TEAMS; team_id++)
    {
        for (int opponent = team_id + 1; opponent < N_TEAMS; opponent++)
        {
            matchup_chances[PACK_MATCHUP(opponent, team_id)] = (
            1 - matchup_chances[PACK_MATCHUP(team_id, opponent)]);
        }
    }
}

void play_round5(Swiss &bracket)
{
    static uint8_t matchups[4];
    bracket.get_matchups(&matchups[0]);

    // play the 3 matches in round 5
    for (uint8_t m = 0; m < 3; m++)
    {
        double p_team0 = matchup_chances[matchups[m]];
        uint8_t team0 = TEAM0(matchups[m]);
        uint8_t team1 = TEAM1(matchups[m]);

        // add probabilities for team0 beating team1
        double p_scenario_team0 = bracket.scenario_probability * p_team0;
        team_p_wl[team0][PACK_WL(3, 2)] += p_scenario_team0;
        team_p_wl[team1][PACK_WL(2, 3)] += p_scenario_team0;

        // add probabilities for team1 beating team0
        double p_scenario_team1 = bracket.scenario_probability * (1-p_team0);
        team_p_wl[team0][PACK_WL(2, 3)] += p_scenario_team1;
        team_p_wl[team1][PACK_WL(3, 2)] += p_scenario_team1;
    }
}

void play(Swiss &bracket, uint8_t* matchups)
{
    // iteration_counter++;
    // if (iteration_counter > 1e7)
    // {
    //     return;
    // }

    static uint8_t matchup_stack[100]; // prety sure we only need 29 or something but im scared
    static uint64_t ms_counter = 0;
    uint64_t n_new_matches = 0;

    // if next matchup is zero, round is done
    if(!matchups[0])
    {
        // bracket.print_standings();
        bracket.round++;

        if(bracket.round >= 5)
        {
            // auto start = std::chrono::high_resolution_clock::now();
            // add probabilities
            for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
            {
                uint64_t wl = bracket.get_wl(team_id);
                team_p_wl[team_id][wl] += bracket.scenario_probability;
            }
            play_round5(bracket);
            // auto end = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<double, std::milli> duration = end - start;
            // p_time += duration.count();
            // exit
            return;
        }

        // add new matchups to the matchup stack
        matchups = &matchup_stack[ms_counter];
        n_new_matches = bracket.get_matchups(matchups);
        ms_counter += n_new_matches;
    }

    // grab the probability of team0 winning this matchup
    double p_team0 = matchup_chances[matchups[0]];

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
        for (int wl = 0xE; wl >= 0x3; wl--)
        {
            double p_wl = team_p_wl[team_id][wl];
            uint8_t wins = WINS(wl);
            uint8_t losses = LOSSES(wl);
            if (wins == WL_MAX || losses == WL_MAX)
            {
                std::cout << " " << +wins << +losses << ": " << team_p_wl[team_id][wl];
            }
        }
        std::cout << std::endl;
    }
}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < (N_TEAMS*N_TEAMS); i++)
    {
        matchup_chances[i] = 0.9f;
    }
    complement_matchup_chances();

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




