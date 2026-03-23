#pragma once

// CS2 Signatures - IDA-style patterns
// Module: client.dll unless noted otherwise

namespace Signatures {

    // Rendering / SwapChain
    constexpr auto swapchain = "48 89 2D ? ? ? ? 66 0F 7F 05 ? ? ? ?";

    // Input System
    constexpr auto csgoinput = "48 8B 0D ? ? ? ? 4C 8B C6 8B 10 E8";
    constexpr auto get_view_angles = "4C 8B C1 85 D2 74 ? 48 8D 05";
    constexpr auto set_view_angles = "85 D2 75 ? 48 63 81";

    // Entity System
    constexpr auto get_entity_index = "E8 ? ? ? ? 8B 8D ? ? ? ? 8D 51";
    constexpr auto get_base_entity = "4C 8D 49 10 81 FA ?? ?? 00 00 77 ?? 8B CA C1 F9 09";
    constexpr auto entity_system = "48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA C1 EB";

    // CreateMove (client.dll)
    constexpr auto create_move = "48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 41 54";
    constexpr auto create_move_mover1 = "48 89 5C 24 ?? 55 57 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 8B 01 48 8B F9";

    // Input Construction
    constexpr auto construct_input_data = "E8 ? ? ? ? 48 8B CF 48 8B F0 44 8B B0 10 59 00 00";
    constexpr auto automake_user_cmd = "E8 ? ? ? ? 48 89 44 24 40 48 8D 4D F0";

    // Rendering / Materials
    constexpr auto set_mesh_group_mask = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71";

    // Game State
    constexpr auto get_is_in_game = "48 8B ? ? ? ? ? 48 85 C0 74 15 80 B8 ? ? ? ? ? 75 0C 83 B8 ? ? ? ? 06";
    constexpr auto get_is_connected = "48 8B 05 ? ? ? ? 48 85 C0 74 ? 80 B8 ? ? ? ? ? 75 ? 83 B8 ? ? ? ? ? 7C";

    // Local Player
    constexpr auto get_local_player_index = "40 53 48 83 EC ? 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 8B DA FF 90 ? ? ? ? 48 8B C3 48 83 C4 ? 5B C3 CC CC CC CC CC CC CC CC CC CC 48 8B 05";
    constexpr auto get_local_pawn = "40 53 48 83 EC ? 33 C9 E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 48 8B 00 48 8B CB FF 90 ? ? ? ? 84 C0 74 ? 48 8B C3";

    // View Matrix
    constexpr auto get_matrix_for_view = "48 8B C4 48 89 68 ? 48 89 70 ? 57 48 81 EC ? ? ? ? 0F 29 70 ? 49 8B F1";
    constexpr auto screen_transform = "48 89 5C 24 08 57 48 83 EC ? 48 83 3D ? ? ? ? ? 48 8B DA";

} // namespace Signatures
