#include <stdint.h>
#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>

#define N_TEAMS 16
#define MAGIC_JOHNSON 0x1111111111111111ULL

#define TEAM_SHIFT(team_id) ((team_id) * 4)
#define INV_TEAM_SHIFT(team_id) ((N_TEAMS - 1 - (team_id))*4)

#define TEAM_FLAG(team_id) (0x1ULL << ((team_id) * 4))
#define INV_TEAM_FLAG(team_id) (TEAM_FLAG(N_TEAMS - 1 - (team_id)))

// Generate a mask for all 4 bit nibbles of a 64 bit int
#define MASK_ALL(mask) (MAGIC_JOHNSON * (mask))

// Win loss packing function
// 0-3 wins, 0-3 losses
// 0b WW LL 
#define PACK_WL(wins, losses) (((wins) << 2) | (losses))
#define WINS(wl) (((wl) >> 2) & 0x3)
#define LOSSES(wl) ((wl) & 0x3)
#define WIN 0x4ULL
#define LOSS 0x1ULL

#define PACK_MATCHUP(team0, team1) (((team1) << 4) | (team0))
#define TEAM0(matchup) ((matchup) & 0xF)
#define TEAM1(matchup) (((matchup) >> 4) & 0xF)

double max_time_ms[5] = {0.0f};
double avg_time_ms[5] = {0.0f};
uint64_t n_time_ms[5] = {0};

int r2s = 0;

std::array<std::string, 16> team_names = {
    "furia",
    "vitality",
    "falcons",
    "mongolz",
    "mouz",
    "spirit",
    "g2", 
    "pain",
    "navi",
    "faze",
    "b8",
    "imperial",
    "parivision",
    "liquid",
    "passion",
    "3dmax",
};

class Seeding {
    private:
        uint64_t seeding[16] = {0};
        // would be nice to refactor so that there is a different seeding object
        // for each WL record, but I think its faster to just have some unused
        // uint64s here and index based on wl rather than use branching

    public:
        void reset(const uint64_t wl)
        {
            seeding[wl] = 0x7F7F7F7F7F7F7F7F;
        }

        void add_team(const uint8_t team_id, const uint64_t wl, const int64_t buchholtz_b)
        {
            // must call reset first!
            if (!seeding[wl])
            {
                // if seeding has not been set to the magic number, 
                // assume this WL is not important
                return;
            }
            //std::cout << "Buch: " << std::hex << buchholtz_b << std::dec << std::endl;
            uint64_t team_seed = ((buchholtz_b & 0xF) << 4 | team_id);
            //std::cout << "Team seed: " << std::hex << team_seed << std::dec << std::endl;

            uint64_t higher_team_mask = 0;
            uint64_t seeding_temp = seeding[wl];

            for (uint8_t i = 0; i < 8; i++)
            {
                if ((int8_t)team_seed < (int8_t)seeding_temp)
                {
                    // make space for the new team
                    seeding[wl] = (seeding[wl] & ~higher_team_mask) << 8 | seeding[wl] & higher_team_mask;
                    // add the new team
                    seeding[wl] |= team_seed << (i*8);

                    break;
                }
                // shift down the temporary seeding for next comparison
                seeding_temp = seeding_temp >> 8;
                // add the current team slot to the higher team mask
                higher_team_mask = higher_team_mask << 8 | 0xFF;
            }

            //std::cout << std::hex << seeding[wl] << std::dec << std::endl;
        }

        uint8_t get_team(const uint64_t wl, const uint8_t seed)
        {
            return seeding[wl] >> (8*seed) & 0xF;
        }   
};

class Swiss {
    private:
        inline static Seeding seeding{};

        int fast_buchholtz_b(uint64_t team_opponents)
        {
            // buchholtz_b is negative buchholtz, more convenient for sorting alg
            uint64_t wins = (team_wl >> 2) & MASK_ALL(0x3);
            uint64_t losses = team_wl & MASK_ALL(0x3);
            uint64_t total_wins = (team_opponents * wins) >> 60;
            uint64_t total_losses = (team_opponents * losses) >> 60;
            int64_t result = (int64_t)(total_losses - total_wins);
            //std::cout << "Buch " << result << std::endl;

            return result;
        }

        void reseed()
        {
            for (int team_id = N_TEAMS-1; team_id >= 0; team_id--)
            {
                uint64_t wl = get_wl(team_id);
                int64_t buchholtz_b = fast_buchholtz_b(get_opponents(team_id));
                seeding.add_team(team_id, wl, buchholtz_b);
            }
        }

        void get_matchups_r45(uint64_t wl, uint8_t matchups[])
        {
            // rounds 4 and 5 use a valve provided lookup table
            // we choose the first set of matchups which does not result in
            // a rematch
            static const uint8_t r45_matchups[15][3][2] = {
                {{0, 5}, {1, 4}, {2, 3}},
                {{0, 5}, {1, 3}, {2, 4}},
                {{0, 4}, {1, 5}, {2, 3}},
                {{0, 4}, {1, 3}, {2, 5}},
                {{0, 3}, {1, 5}, {2, 4}},
                {{0, 3}, {1, 4}, {2, 5}},
                {{0, 5}, {1, 2}, {3, 4}},
                {{0, 4}, {1, 2}, {3, 5}},
                {{0, 2}, {1, 5}, {3, 4}},
                {{0, 2}, {1, 4}, {3, 5}},
                {{0, 3}, {1, 2}, {4, 5}},
                {{0, 2}, {1, 3}, {4, 5}},
                {{0, 1}, {2, 5}, {3, 4}},
                {{0, 1}, {2, 4}, {3, 5}},
                {{0, 1}, {2, 3}, {4, 5}},
            };

            for (uint8_t i = 0; i < 15; i++)
            {
                bool found_rematch = false;
                for (uint8_t j = 0; j < 3; j++)
                {
                    uint8_t team0 = seeding.get_team(wl, r45_matchups[i][j][0]);
                    uint8_t team1 = seeding.get_team(wl, r45_matchups[i][j][1]);
                    if (rematch(team0, team1))
                    {
                        found_rematch = true;
                        break;
                    }
                    matchups[j] = PACK_MATCHUP(team0, team1);
                }
                if (!found_rematch)
                {
                    return;
                }
            }
        }

    public:
        // state variables
        uint64_t team_wl = 0; // packed, stored in reverse order
        uint64_t opponents[4] = {};
        uint8_t round = 1;
        double scenario_probability = 1.0f;

        Swiss() = default;

        Swiss(const Swiss &other)
        {
            team_wl = other.team_wl;
            round = other.round;
            scenario_probability = other.scenario_probability;
            for (uint8_t i = 0; i < 4; i++) {opponents[i] = other.opponents[i];}
        }

        uint64_t get_opponents(const uint8_t team_id)
        {
            return (opponents[team_id % 4] >> (team_id / 4)) & MASK_ALL(0x1);
        }

        uint64_t get_wl(const uint8_t team_id)
        {
            return (team_wl >> INV_TEAM_SHIFT(team_id)) & 0xF;
        }

        bool rematch(const uint8_t team0, const uint8_t team1)
        {
            return (get_opponents(team0) & TEAM_FLAG(team1));
        }

        void add_opponent(const uint8_t team_id, const uint8_t opponent_id)
        {
            opponents[team_id % 4] |= (0x1ULL << TEAM_SHIFT(opponent_id) << (team_id / 4));
        }

        void play_match(const uint8_t winner, const uint8_t loser, const double probability)
        {
            team_wl += WIN << INV_TEAM_SHIFT(winner);
            team_wl += LOSS << INV_TEAM_SHIFT(loser);
            add_opponent(winner, loser);
            add_opponent(loser, winner);
            scenario_probability *= probability;
        }

        // void get_matchups_r45_fast(Seeding &seeding, uint8_t seed_offset, uint8_t matchups[])
        // {
        //     // rounds 4 and 5 use a valve provided lookup table
        //     // we choose the first set of matchups which does not result in
        //     // a rematch
        //     static const uint8_t r45_matchups[15][3][2] = {
        //         {{0, 5}, {1, 4}, {2, 3}},
        //         {{0, 5}, {1, 3}, {2, 4}},
        //         {{0, 4}, {1, 5}, {2, 3}},
        //         {{0, 4}, {1, 3}, {2, 5}},
        //         {{0, 3}, {1, 5}, {2, 4}},
        //         {{0, 3}, {1, 4}, {2, 5}},
        //         {{0, 5}, {1, 2}, {3, 4}},
        //         {{0, 4}, {1, 2}, {3, 5}},
        //         {{0, 2}, {1, 5}, {3, 4}},
        //         {{0, 2}, {1, 4}, {3, 5}},
        //         {{0, 3}, {1, 2}, {4, 5}},
        //         {{0, 2}, {1, 3}, {4, 5}},
        //         {{0, 1}, {2, 5}, {3, 4}},
        //         {{0, 1}, {2, 4}, {3, 5}},
        //         {{0, 1}, {2, 3}, {4, 5}},
        //     };

        //     // grab the teams
        //     uint8_t teams[6] = {};
        //     for (uint8_t seed = 0; seed < 6; seed++)
        //     {
        //         teams[seed] = seeding.get_team(seed);
        //     } 

        //     // store already played matchups in a bit array
        //     // Bits (LSB first):
        //     // 0v1 0v2 0v3 0v4 0v5 1v2 1v3 1v4 1v5 etc.
        //     uint64_t played_matchups = 0;
        //     for (uint8_t seed0 = 0; seed0 < 5; seed0++)
        //     {
        //         uint64_t opponents = get_opponents(teams[seed0]);
        //         for (uint8_t seed1 = seed0 + 1; seed1 < 6; seed1++)
        //         {

        //         }
        //     }


        //     for (uint8_t i = 0; i < 15; i++)
        //     {
        //         bool found_rematch = false;
        //         for (uint8_t j = 0; j < 3; j++)
        //         {
        //             uint8_t team0 = seeding.get_team(r45_matchups[i][j][0] + seed_offset);
        //             uint8_t team1 = seeding.get_team(r45_matchups[i][j][1] + seed_offset);
        //             if (rematch(team0, team1))
        //             {
        //                 found_rematch = true;
        //                 break;
        //             }
        //             matchups[j] = PACK_MATCHUP(team0, team1);
        //         }
        //         if (!found_rematch)
        //         {
        //             return;
        //         }
        //     }
        // }

        uint64_t get_matchups(uint8_t matchups[])
        {
            uint8_t n_matchups = 0;

            double time_ms = 0;
            auto start = std::chrono::high_resolution_clock::now();
        
            // round 1 uses fixed matchups based on pre-stage seed (team_id)
            if (round == 1)
            {
                for (uint8_t i = 0; i < 8; i++)
                {
                    matchups[i] = PACK_MATCHUP(i, i+8);
                }
                n_matchups = 8;
            }
            // In rounds 2 and 3, the higest seed plays the lowest available seed that does not
            // result in a rematch. Interestingly, rematches are impossible in the 1-0, 0-1, 2-0
            // and 0-2 matchups, so we only need to check for rematches in the 1-1 matchups.
            else if (round == 2)
            {
                // keep track of number of round 2s, there are 256 total and this is a nice
                // way to make a progress bar
                r2s++;
                std::cout << r2s << "/256" << std::endl;

                seeding.reset(PACK_WL(1, 0));
                seeding.reset(PACK_WL(0, 1));
                reseed();
                for (uint8_t i = 0; i < 4; i++)
                {
                    // 1-0 matchups
                    uint8_t team0 = seeding.get_team(PACK_WL(1, 0), i);
                    uint8_t team1 = seeding.get_team(PACK_WL(1, 0), 7-i);
                    matchups[i] = PACK_MATCHUP(team0, team1);

                    // 0-1 matchups
                    team0 = seeding.get_team(PACK_WL(0, 1), i);
                    team1 = seeding.get_team(PACK_WL(0, 1), 7-i);
                    matchups[i+4] = PACK_MATCHUP(team0, team1);
                }
                n_matchups = 8;
            }
            else if (round == 3)
            {
                seeding.reset(PACK_WL(2, 0));
                seeding.reset(PACK_WL(1, 1));
                seeding.reset(PACK_WL(0, 2));
                reseed();
                for (uint8_t i = 0; i < 2; i++)
                {
                    // 2-0 matchups
                    uint8_t team0 = seeding.get_team(PACK_WL(2, 0), i);
                    uint8_t team1 = seeding.get_team(PACK_WL(2, 0), 3-i);
                    matchups[i] = PACK_MATCHUP(team0, team1);

                    // 0-2 matchups
                    team0 = seeding.get_team(PACK_WL(0, 2), i);
                    team1 = seeding.get_team(PACK_WL(0, 2), 3-i);
                    matchups[i+6] = PACK_MATCHUP(team0, team1);
                }
                uint8_t team1s[4] = {};
                for (uint8_t i = 0; i < 4; i++)
                {
                    // 1-1 matchups
                    // Only store the high seed team. Save the low seed team in a list.
                    matchups[i+2] = seeding.get_team(PACK_WL(1, 1), i);
                    team1s[i] = seeding.get_team(PACK_WL(1, 1), 7-i);
                }
                // Check for rematches and swap around the low seed teams if one exists.
                // Then add the low seed teams to the matchups.
                for (uint8_t i = 0; i < 4; i++)
                {
                    if (rematch(matchups[i+2], team1s[i]))
                    {
                        matchups[i+2] |= team1s[i+1] << 4;
                        team1s[i+1] = team1s[i];
                    }
                    else
                    {
                        matchups[i+2] |= team1s[i] << 4;
                    }
                }
                n_matchups = 8;
            }
            else if (round == 4)
            {
                seeding.reset(PACK_WL(2, 1));
                seeding.reset(PACK_WL(1, 2));
                reseed();
                // 2-1 matchups
                get_matchups_r45(PACK_WL(2, 1), &matchups[0]);
                // 1-2 matchups
                get_matchups_r45(PACK_WL(1, 2), &matchups[3]);

                n_matchups = 6;

            }
            else if (round == 5)
            {
                seeding.reset(PACK_WL(2, 2));
                reseed();
                // 2-2 matchups
                get_matchups_r45(PACK_WL(2, 2), &matchups[0]);

                n_matchups = 3;
            }

            // add null terminator to end of matchups
            // this is very important to prevent invalid memory access
            matchups[n_matchups] = 0;

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
            time_ms = duration.count();
            if (time_ms > max_time_ms[round-1])
            {
                max_time_ms[round-1] = time_ms;
            }
            avg_time_ms[round-1] += time_ms;
            n_time_ms[round-1]++;

            //std::cout << "Elapsed time: " << duration.count() << " ms" << std::endl;

            // add the null terminator to the count
            return n_matchups + 1;
        }

        static void print_matchups(const uint8_t matchups[])
        {
            std::cout << "Matchups:" << std::endl;
            for (uint8_t i = 0; i < 9; i++)
            {
                if (!matchups[i])
                {
                    break;
                }
                std::cout << team_names[TEAM0(matchups[i])] << " vs " << team_names[TEAM1(matchups[i])] << std::endl;
            }
            std::cout << std::endl;
        }

        void print_standings()
        {
            std::cout << "Standings:" << std::endl;
            for (uint8_t team_id = 0; team_id < 16; team_id++)
            {
                uint64_t wl = get_wl(team_id);
                std::cout << team_names[team_id] << " WL: " << WINS(wl) << LOSSES(wl) << 
                    " B: " << fast_buchholtz_b(get_opponents(team_id)) << std::endl;
            }
            std::cout << std::endl;
        }
};