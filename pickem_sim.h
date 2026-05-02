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

#define PACK_MATCHUP(team0, team1) (((team0) << 4) | (team1))
#define TEAM0(matchup) (((matchup) >> 4) & 0xF)
#define TEAM1(matchup) ((matchup) & 0xF)

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

void print_matchups(uint8_t matchups[])
{
    std::cout << "Matchups:" << std::endl;
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t team0 = TEAM0(matchups[i]);
        uint8_t team1 = TEAM1(matchups[i]);
        if (team0 == team1)
        {
            break;
        }
        std::cout << team_names[team0] << " vs " << team_names[team1] << std::endl;
    }
    std::cout << std::endl;
}

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
};

class Swiss {
    public:
        // state variables
        uint64_t team_wl = 0; // packed, stored in reverse order
        uint64_t opponents[4] = {};
        uint8_t round = 1;
        double scenario_probability = 1.0f;

        // temporary variables
        // uint8_t seeding[8];

        uint64_t pack_wl(uint64_t wins, uint64_t losses)
        {
            return ((wins << 2) | losses);
        }

        uint64_t get_opponents(uint8_t team_id)
        {
            return (opponents[team_id % 4] >> (team_id / 4)) & MASK_ALL(0x1);
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



        void get_matchups(uint8_t matchups[8])
        {
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
                return;
            }

        }
};