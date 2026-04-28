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
        uint64_t win_loss = 0;
        uint64_t opponents[16] = {};
        uint8_t round = 0;

        // temporary variables
        // uint64_t difficulty_score = 0;
        // uint8_t seeding[8];

        uint64_t pack_wl(uint64_t wins, uint64_t losses)
        {
            return ((wins << 2) | losses);
        }

        void set_team_record(uint8_t team_id, uint8_t wins, uint8_t losses)
        {
            win_loss |= pack_wl(wins, losses) << (team_id * 4);
        }

        int fast_buchholtz(uint64_t difficulty_score, uint64_t opponents)
        {
            // TODO: we can probably combine these two operations
            uint64_t fast_buchholtz = (MAGIC_JOHNSON * difficulty_score) >> 60;
            int result = ((int)fast_buchholtz << (sizeof(int) * 8 - 3)) >> (sizeof(int) * 8 - 3);
            return result;
        }

        uint64_t get_difficulty_score()
        {
            uint64_t wins = (win_loss >> 2) & MASK_ALL(0x3);
            uint64_t losses = win_loss & MASK_ALL(0x3);
            // TODO: can we store losses as 2s complement rather than calculating here?
            losses = ((~losses & MASK_ALL(0x7)) + MAGIC_JOHNSON) & MASK_ALL(0x7); // convert losses to 2s complement
            return (wins + losses) & MASK_ALL(0x7); // TODO: can we skip this last step and throw away the bit elsewhere?
        }


    // uint8_t get_buchholtz(uint8_t team_id)
    // {
        
    //     int8_t buchholtz = 0;
    //     for (uint8_t i = 0; i < 16; i++)
    //     {
    //         uint16_t played = (opponents[team_id] >> i) & 0x0001;
    //         buchholtz += played * team_difficulty_score[i];
    //     }
    // }

    // void get_seeding(uint16_t teams, uint8_t (&seeding)[8])
    // {
    //     for (uint8_t team_id = 0; team_id < 16, team_id++)
    //     {

    //     }
    // }
};
