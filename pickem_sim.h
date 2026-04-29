#include <stdint.h>
#include <iostream>

#define N_TEAMS 16
#define MAGIC_JOHNSON 0x1111111111111111ULL

#define TEAM_FLAG(team_id) (0x1ULL << ((team_id) * 4))
#define INV_TEAM_FLAG(team_id) (TEAM_FLAG(N_TEAMS - 1 - team_id))

// Generate a mask for all 4 bit nibbles of a 64 bit int
#define MASK_ALL(mask) (MAGIC_JOHNSON * mask)

// Win loss packing function
// 0-3 wins, 0-3 losses
// 0b WW LL 
#define PACK_WL(wins, losses) ((wins << 2) | losses)

class Swiss {
    public:
        // state variables
        uint64_t team_wins = 0;
        uint64_t team_losses = 0;
        uint64_t opponents[16] = {};
        uint8_t round = 0;

        // temporary variables
        // uint8_t seeding[8];

        uint64_t pack_wl(uint64_t wins, uint64_t losses)
        {
            return ((wins << 2) | losses);
        }

        void set_team_record(uint8_t team_id, uint8_t wins, uint8_t losses)
        {
            team_losses |= losses << (team_id * 4);
        }

        int fast_buchholtz(uint64_t opponents)
        {
            uint64_t total_wins = (opponents * team_wins) >> 60;
            uint64_t total_losses = (opponents * team_losses) >> 60;
            int64_t result = (int64_t)(total_wins - total_losses);

            return result;
        }
};
