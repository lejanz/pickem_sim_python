#include "pickem_points.h"

#include <assert.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iostream>

#define N_PICKS 10
#define PICKS_TO_WIN 5

double team_p_wl[16][3] = {0.0f};

void read_file(std::string odds_path)
{
    std::ifstream file(odds_path);
    assert(file.is_open());

    std::string line;
    for (int i = 0; i < 3; i++)
    {
        assert(std::getline(file, line) && "Not enough rows in file!");
        std::stringstream ss(line);
        std::string value;
        for (int team_id = 0; team_id < 16; team_id++)
        {
            assert(std::getline(ss, value, ' ') && "Not enough columns in csv!");
            team_p_wl[team_id][i] = std::stod(value);
        }
    }
}

double p_coin(std::array<uint8_t, 10> &team_picks)//std::array<uint8_t, 2> picks30, std::array<uint8_t, 6> picks_312, std::array<uint8_t, 2> picks03)
{
    double p_total = 0.0f;
    uint8_t pick[PICKS_TO_WIN] = {0, 1, 2, 3, 4};
    while(pick[0] <= N_PICKS - PICKS_TO_WIN)
    {
        double p_temp = 1.0f;
        for (int i = 0; i < PICKS_TO_WIN; i++)
        {
            double p_this;
            if (pick[i] < 2)
            {
                p_this = team_p_wl[team_picks[pick[i]]][0];
            }
            else if (pick[i] < 8)
            {
                p_this = team_p_wl[team_picks[pick[i]]][1];
            }
            else
            {
                p_this = team_p_wl[team_picks[pick[i]]][2];
            }
            std::cout << p_this << " ";
            p_temp *= p_this;
        }
        std::cout << p_temp << std::endl;
        p_total += p_temp;

        // for (int i = 0; i < 5; i++)
        // {
        //     std::cout << +pick[i];
        // }
        // std::cout << std::endl;
        pick[PICKS_TO_WIN-1]++;
        for (int i = (PICKS_TO_WIN-1); i >= 1; i--)
        {
            if (pick[i] > N_PICKS - PICKS_TO_WIN + i)
            {
                pick[i-1]++;
                for (int j = i; j < PICKS_TO_WIN; j++)
                {
                    pick[j] = pick[j-1] + 1;
                }
            }
        }
    }

    return p_total;


    // for (uint8_t team0 = 0; team0 < 10; team0++)
    // {
    //     for (uint8_t team1 = team0 + 1; team1 < 10; team1++)
    //     {
    //         for (uint8_t team2 = team1 + 1; team2 < 10; team2++)
    //         {
    //             for (uint8_t team3 = team2 + 1; team2 < 10; team2++)
    //         }
    //     }
    // }
}


int main()
{
    std::array<uint8_t, 10> picks = {1, 2, 3, 4, 5, 6, 7, 14, 15};

    read_file("odds.txt");
    std::cout << team_p_wl[0][0] << std::endl;
    // double p = p_coin(picks);
    // std::cout << p << std::endl;
}