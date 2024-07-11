#include "Color.h"
#include "stdafx.h"

#include <ImGuiAddons.h>
#include <string>
#include <Keys.h>

namespace {
    ImGui::ImGuiContextMenuCallback imguiaddons_context_menu_callback = nullptr;
    void* imguiaddons_context_menu_wparam = nullptr;
    const char* imguiaddons_context_menu_id = "###imguiaddons_context_menu";
    ImGui::ImGuiContextMenuCallback imguiaddons_context_menu_pending = nullptr;

    ImGui::ImGuiConfirmDialogCallback imguiaddons_confirm_dialog_callback = nullptr;
    void* imguiaddons_confirm_dialog_wparam = nullptr;
    const char* imguiaddons_confirm_dialog_id = "###imguiaddons_confirm_dialog";
    ImGui::ImGuiConfirmDialogCallback imguiaddons_confirm_dialog_pending = nullptr;
    std::string imguiaddons_confirm_dialog_message = "Are you sure?";

}

namespace ImGui {
    float element_spacing_width = 0.f;
    int element_spacing_cols = 1;
    int element_spacing_col_idx = 0;
    float* element_spacing_indent = nullptr;

    const float& FontScale()
    {
        return GetIO().FontGlobalScale;
    }

    void StartSpacedElements(const float width, const bool include_font_scaling)
    {
        element_spacing_width = width;
        if (include_font_scaling) {
            element_spacing_width *= FontScale();
        }
        element_spacing_cols = static_cast<int>(std::floor(GetContentRegionAvail().x / element_spacing_width));
        element_spacing_col_idx = 0;
        element_spacing_indent = &GetCurrentWindow()->DC.Indent.x;
    }

    void NextSpacedElement()
    {
        if (element_spacing_col_idx) {
            if (element_spacing_col_idx < element_spacing_cols) {
                SameLine(element_spacing_width * element_spacing_col_idx + *element_spacing_indent);
            }
            else {
                element_spacing_col_idx = 0;
            }
        }
        element_spacing_col_idx++;
    }

    void SetContextMenu(ImGuiContextMenuCallback callback, void* wparam) {
        imguiaddons_context_menu_pending = callback;
        imguiaddons_context_menu_wparam = wparam;

    }

    void DrawContextMenu() {
        if (imguiaddons_context_menu_pending) {
            imguiaddons_context_menu_callback = imguiaddons_context_menu_pending;
            imguiaddons_context_menu_pending = nullptr;
            ImGui::OpenPopup(imguiaddons_context_menu_id);
            return;
        }
        if (!ImGui::BeginPopup(imguiaddons_context_menu_id))
            return;
        if (!(imguiaddons_context_menu_callback && imguiaddons_context_menu_callback(imguiaddons_context_menu_wparam))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    void ShowHelp(const char* help)
    {
        SameLine();
        TextDisabled("%s", "(?)");
        if (IsItemHovered()) {
            SetTooltip("%s", help);
        }
    }

    void TextShadowed(const char* label, const ImVec2 offset, const ImVec4& shadow_color)
    {
        const ImVec2 pos = GetCursorPos();
        ImGui::PushStyleColor(ImGuiCol_Text, shadow_color);
        SetCursorPos(ImVec2(pos.x + offset.x, pos.y + offset.y));
        TextUnformatted(label);
        ImGui::PopStyleColor();
        SetCursorPos(pos);
        TextUnformatted(label);
    }

    void SetNextWindowCenter(const ImGuiWindowFlags flags)
    {
        const auto& io = GetIO();
        SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), flags, ImVec2(0.5f, 0.5f));
    }

    void DrawConfirmDialog() {
        if (imguiaddons_confirm_dialog_pending) {
            imguiaddons_confirm_dialog_callback = imguiaddons_confirm_dialog_pending;
            imguiaddons_confirm_dialog_pending = nullptr;
            ImGui::OpenPopup(imguiaddons_confirm_dialog_id);
            return;
        }
        if (!ImGui::BeginPopupModal(imguiaddons_confirm_dialog_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (imguiaddons_confirm_dialog_callback) {
                imguiaddons_confirm_dialog_callback(false, imguiaddons_confirm_dialog_wparam);
                imguiaddons_confirm_dialog_callback = nullptr;
            }
            return;
        }
        static bool was_enter_down = true, was_escape_down = true;
        if (ImGui::IsWindowAppearing()) {
            was_enter_down = ImGui::IsKeyDown(ImGuiKey_Enter);
            was_escape_down = ImGui::IsKeyDown(ImGuiKey_Escape);
        }
        bool is_enter_down = ImGui::IsKeyDown(ImGuiKey_Enter);
        bool is_escape_down = ImGui::IsKeyDown(ImGuiKey_Escape);
        ImGui::TextUnformatted(imguiaddons_confirm_dialog_message.c_str());

        if (ImGui::Button("Yes", ImVec2(120, 0)) || (!was_enter_down && is_enter_down)) {
            ImGui::CloseCurrentPopup();
            imguiaddons_confirm_dialog_callback(true, imguiaddons_confirm_dialog_wparam);
            imguiaddons_confirm_dialog_callback = nullptr;
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0)) || (!was_escape_down && is_escape_down)) {
            ImGui::CloseCurrentPopup();
            imguiaddons_confirm_dialog_callback(false, imguiaddons_confirm_dialog_wparam);
            imguiaddons_confirm_dialog_callback = nullptr;
        }
        was_escape_down = is_escape_down;
        was_enter_down = is_enter_down;
        ImGui::EndPopup();
    }

    void ConfirmDialog(const char* message, ImGui::ImGuiConfirmDialogCallback callback, void* wparam)
    {
        imguiaddons_confirm_dialog_message = message;
        imguiaddons_confirm_dialog_pending = callback;
        imguiaddons_confirm_dialog_wparam = wparam;
    }

    bool SmallConfirmButton(const char* label, const char* confirm_content, ImGui::ImGuiConfirmDialogCallback callback, void* wparam)
    {
        static char id_buf[128];
        snprintf(id_buf, sizeof(id_buf), "%s##confirm_popup%p", label, label);
        if (SmallButton(label)) {
            ConfirmDialog(confirm_content, callback, wparam);
            return true;
        }
        return false;
    }

    bool ChooseKey(const char* label, char* buf, size_t buf_len, long* output_key_code)
    {
        ImGui::InputText(label, buf, buf_len, ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_AutoSelectAll);
        if (ImGui::IsItemFocused()) {
            const auto keys = GetPressedKeys();
            if (!keys.empty()) {
                strncpy(buf, KeyName(keys[0]), buf_len);
                *output_key_code = keys[0];
            }
            return true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_TIMES "##clearkeyinput")) {
            strcpy(buf, "None");
            *output_key_code = 0;
            return true;
        }
        return false;
    }

    const std::vector<ImGuiKey>& GetPressedKeys()
    {
        static std::vector<ImGuiKey> pressedKeys;
        static int frameCount = -1;

        int newFrameCount = ImGui::GetFrameCount();
        if (frameCount != newFrameCount) {
            frameCount = newFrameCount;

            pressedKeys.clear();
            for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
                if (ImGui::IsKeyPressed((ImGuiKey)key))
                    pressedKeys.push_back((ImGuiKey)key);
        }

        return pressedKeys;
    }

    bool ConfirmButton(const char* label, bool* confirm_bool, const char* confirm_content)
    {
        static char id_buf[128];
        snprintf(id_buf, sizeof(id_buf), "%s##confirm_popup%p", label, confirm_bool);
        if (Button(label)) {
            OpenPopup(id_buf);
        }
        if (BeginPopupModal(id_buf, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            Text(confirm_content);
            if (Button("OK", ImVec2(120, 0))) {
                *confirm_bool = true;
                CloseCurrentPopup();
            }
            SameLine();
            if (Button("Cancel", ImVec2(120, 0))) {
                CloseCurrentPopup();
            }
            EndPopup();
        }
        return *confirm_bool;
    }

    void ClosePopup(const char* popup_id)
    {
        if (IsPopupOpen(popup_id))
            ClosePopup(popup_id);
    }

    bool CompositeIconButton(const char* label, const ImTextureID* icons, size_t icons_len, const ImVec2& size, const ImGuiButtonFlags flags, const ImVec2& icon_size, const ImVec2& uv0, ImVec2 uv1)
    {
        char button_id[128];
        sprintf(button_id, "###icon_button_%s", label);
        const ImVec2& pos = GetCursorScreenPos();
        const ImVec2& textsize = CalcTextSize(label);
        const bool clicked = ButtonEx(button_id, size, flags);

        const ImVec2& button_size = GetItemRectSize();
        ImVec2 img_size = icon_size;
        if (icon_size.x > 0.f) {
            img_size.x = icon_size.x;
        }
        if (icon_size.y > 0.f) {
            img_size.y = icon_size.y;
        }
        if (img_size.y == 0.f) {
            img_size.y = button_size.y - 2.f;
        }
        if (img_size.x == 0.f) {
            img_size.x = img_size.y;
        }
        const ImGuiStyle& style = GetStyle();
        const float content_width = img_size.x + textsize.x + style.FramePadding.x * 2.f;
        float content_x = pos.x + style.FramePadding.x;
        if (content_width < button_size.x) {
            const float avail_space = button_size.x - content_width;
            content_x += avail_space * style.ButtonTextAlign.x;
        }
        const float img_x = content_x;
        const float img_y = pos.y + (button_size.y - img_size.y) / 2.f;
        const float text_x = img_x + img_size.x + 3.f;
        const float text_y = pos.y + (button_size.y - textsize.y) * style.ButtonTextAlign.y;
        const auto top_left = ImVec2(img_x, img_y);
        const auto bottom_right = ImVec2(img_x + img_size.x, img_y + img_size.y);
        for (size_t i = 0; i < icons_len; i++) {
            if (!icons[i])
                continue;
            if (uv0.x == uv1.x && uv0.y == uv1.y) {
                GetWindowDrawList()->AddImage(icons[i], top_left, bottom_right, uv0, CalculateUvCrop(icons[i], img_size));
            }
            else {
                GetWindowDrawList()->AddImage(icons[i], top_left, bottom_right, uv0, uv1);
            }
        }
        if (label) {
            GetWindowDrawList()->AddText(ImVec2(text_x, text_y), ImColor(GetStyle().Colors[ImGuiCol_Text]), label);
        }
        return clicked;
    }

    bool IconButton(const char* label, const ImTextureID icon, const ImVec2& size, const ImGuiButtonFlags flags, const ImVec2& icon_size)
    {
        return CompositeIconButton(label, &icon, 1, size, flags, icon_size);
    }

    bool ColorButtonPicker(const char* label, Color* imcol, const ImGuiColorEditFlags flags)
    {
        return Colors::DrawSettingHueWheel(label, imcol,
                                           flags | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoLabel |
                                           ImGuiColorEditFlags_NoInputs);
    }

    bool MyCombo(const char* label, const char* preview_text, int* current_item, bool (*items_getter)(void*, int, const char**),
                 void* data, const int items_count)
    {
        ImGuiContext& g = *GImGui;
        constexpr float word_building_delay = .5f; // after this many seconds, typing will make a new search

        if (*current_item >= 0 && *current_item < items_count) {
            items_getter(data, *current_item, &preview_text);
        }

        // this is actually shared between all combos. It's kinda ok because there is
        // only one combo open at any given time, however it causes a problem where
        // if you open combo -> keyboard select (but no enter) and close, the
        // keyboard_selected will stay as-is when re-opening the combo, or even others.
        static int keyboard_selected = -1;

        if (!BeginCombo(label, preview_text)) {
            return false;
        }

        GetIO().WantTextInput = true;
        static char word[64] = "";
        static float time_since_last_update = 0.0f;
        time_since_last_update += g.IO.DeltaTime;
        bool update_keyboard_match = false;
        for (auto n = 0; n < g.IO.InputQueueCharacters.size() && g.IO.InputQueueCharacters[n]; n++) {
            if (const auto c = static_cast<unsigned int>(g.IO.InputQueueCharacters[n])) {
                if (c == ' '
                    || (c >= '0' && c <= '9')
                    || (c >= 'A' && c <= 'Z')
                    || (c >= 'a' && c <= 'z')) {
                    // build temporary word
                    if (time_since_last_update < word_building_delay) {
                        // append
                        const size_t i = strnlen(word, 64);
                        if (i + 1 < 64) {
                            word[i] = static_cast<char>(c);
                            word[i + 1] = '\0';
                        }
                    }
                    else {
                        // reset
                        word[0] = static_cast<char>(c);
                        word[1] = '\0';
                    }
                    time_since_last_update = 0.0f;
                    update_keyboard_match = true;
                }
            }
        }

        // find best keyboard match
        int best = -1;
        bool keyboard_selected_now = false;
        if (update_keyboard_match) {
            for (auto i = 0; i < items_count; i++) {
                const char* item_text;
                if (items_getter(data, i, &item_text)) {
                    int j = 0;
                    while (word[j] && item_text[j] && tolower(item_text[j]) == tolower(word[j])) {
                        ++j;
                    }
                    if (best < j) {
                        best = j;
                        keyboard_selected = i;
                        keyboard_selected_now = true;
                    }
                }
            }
        }

        if (IsKeyPressed(ImGuiKey_Enter) && keyboard_selected >= 0) {
            *current_item = keyboard_selected;
            keyboard_selected = -1;
            CloseCurrentPopup();
            EndCombo();
            return true;
        }
        if (IsKeyPressed(ImGuiKey_UpArrow) && keyboard_selected > 0) {
            --keyboard_selected;
            keyboard_selected_now = true;
        }
        if (IsKeyPressed(ImGuiKey_DownArrow) && keyboard_selected < items_count - 1) {
            ++keyboard_selected;
            keyboard_selected_now = true;
        }

        // Display items
        bool value_changed = false;
        for (auto i = 0; i < items_count; i++) {
            PushID(reinterpret_cast<void*>(i));
            const bool item_selected = i == *current_item;
            const bool item_keyboard_selected = i == keyboard_selected;
            const char* item_text;
            if (!items_getter(data, i, &item_text)) {
                item_text = "*Unknown item*";
            }
            if (Selectable(item_text, item_selected || item_keyboard_selected)) {
                value_changed = true;
                *current_item = i;
                keyboard_selected = -1;
            }
            if (item_selected && IsWindowAppearing()) {
                SetScrollHereY();
            }
            if (item_keyboard_selected && keyboard_selected_now) {
                SetScrollHereY();
            }
            PopID();
        }

        EndCombo();
        return value_changed;
    }

    ImVec2 CalculateUvCrop(const ImTextureID user_texture_id, const ImVec2& size)
    {
        ImVec2 uv1 = {1.f, 1.f};
        if (user_texture_id) {
            const auto texture = static_cast<IDirect3DTexture9*>(user_texture_id);
            D3DSURFACE_DESC desc;
            const HRESULT res = texture->GetLevelDesc(0, &desc);
            if (!SUCCEEDED(res)) {
                return uv1; // Don't throw anything into the log here; this function is called every frame by modules that use it!
            }
            const float ratio = size.x / size.y;
            const float image_ratio = static_cast<float>(desc.Width) / static_cast<float>(desc.Height);
            if (image_ratio < ratio) {
                // Image is taller than the required crop; remove bottom of image to fit.
                uv1.y = ratio * image_ratio;
            }
            else if (image_ratio > ratio) {
                // Image is wider than the required crop; remove right of image to fit.
                uv1.x = ratio / image_ratio;
            }
        }
        return uv1;
    }

    // Given a texture, sprite size in px and the offset of the sprite we want, fill out uv0 and uv1 coords for percentage offsets. False on failure.
    bool GetSpriteUvCoords(const ImTextureID user_texture_id, const ImVec2& single_sprite_size, uint32_t sprite_offset[2], ImVec2* uv0_out, ImVec2* uv1_out)
    {
        if (!user_texture_id)
            return false;
        const auto texture = static_cast<IDirect3DTexture9*>(user_texture_id);
        D3DSURFACE_DESC desc;
        const HRESULT res = texture->GetLevelDesc(0, &desc);
        if (!SUCCEEDED(res)) {
            return false; // Don't throw anything into the log here; this function is called every frame by modules that use it!
        }

        ImVec2 img_dimensions = {static_cast<float>(desc.Width), static_cast<float>(desc.Height)};

        ImVec2 start_px_offset = {single_sprite_size.x * sprite_offset[0], single_sprite_size.y * sprite_offset[1]};
        if (start_px_offset.x >= img_dimensions.x
            || start_px_offset.y >= img_dimensions.y) {
            return false;
        }
        ImVec2 end_px_offset = {start_px_offset.x + single_sprite_size.x, start_px_offset.y + single_sprite_size.y};
        if (end_px_offset.x >= img_dimensions.x
            || end_px_offset.y >= img_dimensions.y) {
            return false;
        }
        uv0_out->x = start_px_offset.x / img_dimensions.x;
        uv0_out->y = end_px_offset.y / img_dimensions.y;

        uv1_out->x = end_px_offset.x / img_dimensions.x;
        uv1_out->x = end_px_offset.y / img_dimensions.y;
        return true;
    }

    void ImageCropped(const ImTextureID user_texture_id, const ImVec2& size)
    {
        Image(user_texture_id, size, {0, 0}, CalculateUvCrop(user_texture_id, size));
    }

    bool ImageButton(ImTextureID user_texture_id, const ImVec2& image_size, const ImVec2& uv0, const ImVec2& uv1, int, const ImVec4& bg_col, const ImVec4& tint_col) {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        if (window->SkipItems)
            return false;

        return ImageButtonEx(window->GetID(user_texture_id ? user_texture_id : window), user_texture_id, image_size, uv0, uv1, bg_col, tint_col);
    }
    bool IsKeyDown(long key) {
        return IsKeyDown(static_cast<ImGuiKey>(key));
    }

    bool IsMouseInRect(const ImVec2& top_left, const ImVec2& bottom_right)
    {
        const ImRect rect(top_left, bottom_right);
        return rect.Contains(GetIO().MousePos);
    }

    void AddImageCropped(const ImTextureID user_texture_id, const ImVec2& top_left, const ImVec2& bottom_right)
    {
        const ImVec2 size = {bottom_right.x - top_left.x, bottom_right.y - top_left.y};
        GetWindowDrawList()->AddImage(user_texture_id, top_left, bottom_right, {0, 0}, CalculateUvCrop(user_texture_id, size));
    }

    bool ColorPalette(const char* label, size_t* palette_index, const ImVec4* palette, const size_t count, const size_t max_per_line, const ImGuiColorEditFlags flags)
    {
        PushID(label);
        BeginGroup();

        bool value_changed = false;
        for (size_t i = 0; i < count; i++) {
            PushID(i);
            if (ColorButton("", palette[i])) {
                *palette_index = i;
                value_changed = true;
            }
            PopID();
            if ((i + 1) % max_per_line != 0) {
                SameLine();
            }
        }

        if (flags & ImGuiColorEditFlags_AlphaPreview) {
            constexpr ImVec4 col;
            PushID(count);
            if (ColorButton("", col, ImGuiColorEditFlags_AlphaPreview)) {
                *palette_index = count;
                value_changed = true;
            }
            PopID();
        }

        EndGroup();
        PopID();
        return value_changed;
    }

    // Store original positions of the windows
    std::unordered_map<std::string_view, ImVec2> original_positions;

    void ClampWindowToScreen(ImGuiWindow* window)
    {
        ImVec2 window_pos = window->Pos;                        // Get the current window position
        const ImVec2 window_size = window->Size;                // Get the current window size
        const ImVec2 display_size = ImGui::GetIO().DisplaySize; // Get the display size
        const std::string_view window_name = window->Name;      // Get the window name

        // Check if the window position needs to be clamped based on the original position if available
        const ImVec2 original_pos = original_positions.contains(window_name) ? original_positions[window_name] : window_pos;

        // Determine if clamping is needed
        const bool needs_clamping = original_pos.x + window_size.x > display_size.x ||
                                    original_pos.y + window_size.y > display_size.y;

        if (needs_clamping) {
            // Save the original position if not already saved
            if (!original_positions.contains(window_name)) {
                original_positions[window_name] = window_pos;
            }

            // Clamp window position to ensure the entire content is on screen
            if (window_pos.x + window_size.x > display_size.x) window_pos.x = display_size.x - window_size.x;
            if (window_pos.y + window_size.y > display_size.y) window_pos.y = display_size.y - window_size.y;

            // Set the new window position
            ImGui::SetWindowPos(window, window_pos, ImGuiCond_Always);
            if (window->Collapsed) {
                original_positions[window_name] = window_pos;
            }
        }
        else {
            const bool is_moving_window = ImGui::GetIO().WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left);
            if (!is_moving_window && original_positions.contains(window_name)) {
                const ImVec2& stored_pos = original_positions.at(window_name);
                if (window_pos.x != stored_pos.x || window_pos.y != stored_pos.y) {
                    ImGui::SetWindowPos(window, original_pos, ImGuiCond_Always);
                    original_positions.erase(window_name);
                }
                else {
                    original_positions[window_name] = window_pos;
                }
            }
        }
    }

    void ClampAllWindowsToScreen(const bool clamp)
    {
        if (clamp) {
            for (const auto window : ImGui::GetCurrentContext()->Windows) {
                // if (window->Collapsed || !window->Active) continue;
                ClampWindowToScreen(window);
            }
        }
        else {
            // restore positions and delete the original position if it's restored
            for (auto it = original_positions.begin(); it != original_positions.end();) {
                ImGui::SetWindowPos(it->first.data(), it->second, ImGuiCond_Always);
                it = original_positions.erase(it);
            }
        }
    }
    bool ButtonWithHint(const char* label, const char* tooltip, const ImVec2& size_arg)
    {
        bool clicked = ButtonEx(label, size_arg, ImGuiButtonFlags_None);
        if (IsItemHovered() && tooltip)
            SetTooltip(tooltip);
        return clicked;
    }
}
