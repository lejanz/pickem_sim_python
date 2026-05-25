#include "pickem_sim.h"

#include "pickem_points.h"

#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <string>

#define WL_MAX 3
#define ROUNDS 5
#define MIN_CHANCE 0.01f

// #define ITERATION_CAP 1e8

uint64_t iteration_counter = 0;
double matchup_chances[N_TEAMS*N_TEAMS] = {};
double team_p_wl[N_TEAMS][16] = {0.0f};

double p_time = 0.0f;

void read_csv(std::string csv_path)
{
    std::ifstream file(csv_path);
    assert(file.is_open());

    std::string line;
    for (int team_id = 0; team_id < N_TEAMS - 1; team_id++)
    {
        assert(std::getline(file, line) && "Not enough rows in csv!");
        std::stringstream ss(line);
        std::string value;
        for (int opponent = 0; opponent < N_TEAMS; opponent++)
        {
            assert(std::getline(ss, value, '\t') && "Not enough columns in csv!");
            if (!value.empty())
            {
                double p_win = std::stod(value);
                // std::cout << team_names[team_id] << " beats " << team_names[opponent] << ": " << p_win << std::endl;
                matchup_chances[PACK_MATCHUP(team_id, opponent)] = p_win;
                matchup_chances[PACK_MATCHUP(opponent, team_id)] = 1 - p_win;
            }
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
    #ifdef ITERATION_CAP
    iteration_counter++;
    if (iteration_counter > ITERATION_CAP)
    {
        return;
    }
    #endif

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
            // add probabilities
            for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
            {
                uint64_t wl = bracket.get_wl(team_id);
                team_p_wl[team_id][wl] += bracket.scenario_probability;
            }
            play_round5(bracket);
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
    std::cout << std::setprecision(2);

    // print all probabilities
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

    std::ofstream outFile("odds.txt");
    if (!outFile.is_open())
    {
        std::cout << "failed to open odds.txt!" << std::endl;
    }

    std::cout << "3-0 chances:" << std::endl;
    for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
    {
        double p = team_p_wl[team_id][PACK_WL(3, 0)];
        std::cout << team_names[team_id] << ": " <<  p << " ";
        outFile << p << " ";
    }
    std::cout << std::endl;
    outFile << std::endl;

    std::cout << "3-2/3-1 chances:" << std::endl;
    for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
    {
        double p = team_p_wl[team_id][PACK_WL(3, 1)] + team_p_wl[team_id][PACK_WL(3, 2)];
        std::cout << team_names[team_id] << ": " <<  p << " ";
        outFile << p << " ";
    }
    std::cout << std::endl;
    outFile << std::endl;

    std::cout << "0-3 chances:" << std::endl;
    for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
    {
        double p = team_p_wl[team_id][PACK_WL(0, 3)];
        std::cout << team_names[team_id] << ": " <<  p << " ";
        outFile << p << " ";
    }
    std::cout << std::endl;
    outFile << std::endl;

}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    read_csv("WinProbability.csv");

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

    // for (uint8_t round = 0; round < 5; round++)
    // {
    //     std::cout << "Round " << 1 + round << ": n=" << n_time_ms[round] << ", " << avg_time_ms[round]/n_time_ms[round] 
    //     << "ms avg, " << avg_time_ms[round] << "ms total" << std::endl;  
    // }

    // std::cout << "Seeding time: " << t_seed << "ms " << count_seed << std::endl;
    // std::cout << "r45 time: " << t_match_r45 << "ms " << count_r45 << std::endl;
    // std::cout << "max depth: " << max_depth << ", avg: " << (double)total_depth / (double)(count_r45) << std::endl;
    std::array<uint8_t, 10> picks = {0, 1, 2, 3, 4, 5, 6, 7, 14, 15};

    return 0;
}




