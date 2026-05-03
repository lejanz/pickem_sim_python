#include <stdint.h>
#include <iostream>
#include <array>
#include <algorithm>

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
        using seed_t = std::tuple<int64_t, uint64_t, int64_t, uint64_t>;
        std::array<seed_t, N_TEAMS> seeding = {};

    public:
        void set_team(uint8_t team_id, uint64_t wins, uint64_t losses, int64_t buchholtz)
        {
            std::get<0>(seeding[team_id]) = -1*(int64_t)wins; // wins descending
            std::get<1>(seeding[team_id]) = losses; // losses ascending
            std::get<2>(seeding[team_id]) = -1*buchholtz; // buchholtz descending
            std::get<3>(seeding[team_id]) = team_id; // team id ascending
        }

        void reseed()
        {
            std::sort(seeding.begin(), seeding.end());
        }

        uint8_t get_team(uint8_t seed)
        {
            // team ID is stored in slot 3
            return std::get<3>(seeding[seed]);
        }

        void print()
        {
            std::cout << "Standings:" << std::endl;
            for (uint8_t seed = 0; seed < 16; seed++)
            {
                std::cout << team_names[get_team(seed)] << " WL: " << -1*std::get<0>(seeding[seed]) << std::get<1>(seeding[seed]) << 
                    " B: " << -1*std::get<2>(seeding[seed]) << std::endl;
                //std::cout << std::hex << opponents[team_id] << std::dec << std::endl;
            }
            std::cout << std::endl;
        }
};

class Swiss {
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

        uint64_t pack_wl(uint64_t wins, uint64_t losses)
        {
            return ((wins << 2) | losses);
        }

        uint64_t get_opponents(uint8_t team_id)
        {
            return (opponents[team_id % 4] >> (team_id / 4)) & MASK_ALL(0x1);
        }

        bool rematch(uint8_t team0, uint8_t team1)
        {
            return (get_opponents(team0) & TEAM_FLAG(team1));
        }

        void add_opponent(uint8_t team_id, uint8_t opponent_id)
        {
            opponents[team_id % 4] |= (0x1ULL << TEAM_SHIFT(opponent_id) << (team_id / 4));
        }

        void set_team_record(uint8_t team_id, uint8_t wins, uint8_t losses)
        {
            team_wl |= pack_wl(wins, losses) << ((15-team_id)*4);
        }

        void play_match(uint8_t winner, uint8_t loser, double probability)
        {
            team_wl += WIN << INV_TEAM_SHIFT(winner);
            team_wl += LOSS << INV_TEAM_SHIFT(loser);
            add_opponent(winner, loser);
            add_opponent(loser, winner);
            scenario_probability *= probability;
        }

        int fast_buchholtz(uint64_t team_opponents)
        {
            uint64_t wins = (team_wl >> 2) & MASK_ALL(0x3);
            uint64_t losses = team_wl & MASK_ALL(0x3);
            uint64_t total_wins = (team_opponents * wins) >> 60;
            uint64_t total_losses = (team_opponents * losses) >> 60;
            int64_t result = (int64_t)(total_wins - total_losses);
            //std::cout << "Buch " << result << std::endl;

            return result;
        }

        void print_standings()
        {
            std::cout << "Standings:" << std::endl;
            for (uint8_t team_id = 0; team_id < 16; team_id++)
            {
                uint64_t wl = team_wl >> INV_TEAM_SHIFT(team_id);
                std::cout << team_names[team_id] << " WL: " << WINS(wl) << LOSSES(wl) << 
                    " B: " << fast_buchholtz(get_opponents(team_id)) << std::endl;
                //std::cout << std::hex << opponents[team_id] << std::dec << std::endl;
            }
            std::cout << std::endl;
        }

        void get_matchups_r45(Seeding &seeding, uint8_t seed_offset, uint8_t matchups[])
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
                    uint8_t team0 = seeding.get_team(r45_matchups[i][j][0] + seed_offset);
                    uint8_t team1 = seeding.get_team(r45_matchups[i][j][1] + seed_offset);
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

        void get_matchups(uint8_t matchups[9])
        {
            // matchup[8] should always be set to zero, this null termination is very important
            // to prevent bad memory access down the line lol
            matchups[8] = 0;
        
            // round 1 uses fixed matchups based on pre-stage seed (team_id)
            if (round == 1)
            {
                for (uint8_t i = 0; i < 8; i++)
                {
                    matchups[i] = PACK_MATCHUP(i, i+8);
                }

                return;
            }

            // beyond round 1, we need to do fancy seeding
            Seeding seeding;
            for (uint8_t team_id = 0; team_id < N_TEAMS; team_id++)
            {
                uint64_t wl = team_wl >> INV_TEAM_SHIFT(team_id);
                // NOTE: we do not need to calculate buchholtz in round 2
                int64_t buchholtz = fast_buchholtz(get_opponents(team_id));
                seeding.set_team(team_id, WINS(wl), LOSSES(wl), buchholtz);
            }
            seeding.reseed();
            //seeding.print();

            // In rounds 2 and 3, the higest seed plays the lowest available seed that does not
            // result in a rematch. Interestingly, rematches are impossible in the 1-0, 0-1, 2-0
            // and 0-2 matchups, so we only need to check for rematches in the 1-1 matchups.
            if (round == 2)
            {
                for (uint8_t i = 0; i < 4; i++)
                {
                    // 1-0 matchups
                    uint8_t team0 = seeding.get_team(i);
                    uint8_t team1 = seeding.get_team(7-i);
                    matchups[i] = PACK_MATCHUP(team0, team1);

                    // 0-1 matchups
                    team0 = seeding.get_team(i+8);
                    team1 = seeding.get_team(15-i);
                    matchups[i+4] = PACK_MATCHUP(team0, team1);
                }
                
                return;
            }

            if (round == 3)
            {
                for (uint8_t i = 0; i < 2; i++)
                {
                    // 2-0 matchups
                    uint8_t team0 = seeding.get_team(i);
                    uint8_t team1 = seeding.get_team(3-i);
                    matchups[i] = PACK_MATCHUP(team0, team1);

                    // 0-2 matchups
                    team0 = seeding.get_team(i+12);
                    team1 = seeding.get_team(15-i);
                    matchups[i+6] = PACK_MATCHUP(team0, team1);
                }

                uint8_t team1s[4] = {};
                for (uint8_t i = 0; i < 4; i++)
                {
                    // 1-1 matchups
                    // Only store the high seed team. Save the low seed team in a list.
                    matchups[i+2] = seeding.get_team(i+4);
                    team1s[i] = seeding.get_team(11-i);
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

                return;
            }

            if (round == 4)
            {
                // 2-1 matchups
                get_matchups_r45(seeding, 2, &matchups[0]);
                // 1-2 matchups
                get_matchups_r45(seeding, 8, &matchups[3]);
                // null terminator
                matchups[6] = 0;

                return;
            }

            if (round == 5)
            {
                // 2-2 matchups
                get_matchups_r45(seeding, 5, &matchups[0]);
                // null terminator
                matchups[3] = 0;
            }
        }

        void print_matchups(uint8_t matchups[])
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
};