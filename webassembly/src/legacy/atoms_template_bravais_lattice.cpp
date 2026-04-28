// Bravais 격자 템플릿
void AtomsTemplate::renderCrystalTemplate() {
    ImGui::Text("Bravais Lattice Templates");
    
    // 격자 템플릿 목록
    static const char* latticeNames[] = {
        "Simple Cubic (SC)",
        "Body-Centered Cubic (BCC)",
        "Face-Centered Cubic (FCC)",
        "Simple Tetragonal (ST)",
        "Body-Centered Tetragonal (BCT)",
        "Simple Orthorhombic (SO)",
        "Body-Centered Orthorhombic (BCO)",
        "Face-Centered Orthorhombic (FCO)",
        "Base-Centered Orthorhombic (BCO/C)",
        "Simple Monoclinic (SM)",
        "Base-Centered Monoclinic (BCM)",
        "Triclinic",
        "Rhombohedral",
        "Hexagonal"
    };
    
    // 사용 가능한 창 너비 계산
    float availWidth = ImGui::GetContentRegionAvail().x;
    
    // 입력 필드 크기 및 간격 계산
    float buttonWidth = availWidth * 0.25f; // 25% 버튼
    float paramWidth = availWidth * 0.65f;  // 65% 파라미터 입력
    float inputWidth = 60.0f;              // 입력 필드 너비
    
    // 각 Bravais 격자 유형에 대한 표시 여부 배열
    bool latticeVisible[14] = {true, true, true, true, true, true, true, true, true, true, true, true, true, true};
    
    // 격자 필터링 옵션
    ImGui::Text("Filter by Crystal System:");
    
    if (ImGui::BeginTable("CrystalSystemFilters", 7, ImGuiTableFlags_SizingFixedFit)) {

        ImGui::TableNextColumn();
        static bool showCubic = true;
        ImGui::Checkbox("Cubic", &showCubic);

        ImGui::TableNextColumn();
        static bool showTetragonal = false;
        ImGui::Checkbox("Tetragonal", &showTetragonal);

        ImGui::TableNextColumn();
        static bool showOrthorhombic = false;
        ImGui::Checkbox("Orthorhombic", &showOrthorhombic);

        ImGui::TableNextColumn();
        static bool showMonoclinic = false;
        ImGui::Checkbox("Monoclinic", &showMonoclinic);

        ImGui::TableNextColumn();
        static bool showTriclinic = false;
        ImGui::Checkbox("Triclinic", &showTriclinic);

        ImGui::TableNextColumn();
        static bool showRhombohedral = false;
        ImGui::Checkbox("Rhombohedral", &showRhombohedral);

        ImGui::TableNextColumn();
        static bool showHexagonal = false;
        ImGui::Checkbox("Hexagonal", &showHexagonal);
        
        ImGui::EndTable();
        
        // 필터 적용
        latticeVisible[0] = latticeVisible[1] = latticeVisible[2] = showCubic;
        latticeVisible[3] = latticeVisible[4] = showTetragonal;
        latticeVisible[5] = latticeVisible[6] = latticeVisible[7] = latticeVisible[8] = showOrthorhombic;
        latticeVisible[9] = latticeVisible[10] = showMonoclinic;
        latticeVisible[11] = showTriclinic;
        latticeVisible[12] = showRhombohedral;
        latticeVisible[13] = showHexagonal;
    }
    
    ImGui::Separator();
    
    // 변경된 InputFloat 호출 예시 (모든 InputFloat 호출에 적용)
    if (ImGui::BeginTable("BravaisLatticeTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, buttonWidth);
        ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);
        
        // 각 격자 유형별 행
        for (int i = 0; i < 14; i++) {
            if (!latticeVisible[i]) continue;  // 필터링 적용
            
            ImGui::TableNextRow();
            
            // 첫 번째 열: 버튼
            ImGui::TableNextColumn();
            if (ImGui::Button(latticeNames[i], ImVec2(-FLT_MIN, 0))) {
                // 격자 선택
                setBravaisLattice(i);
            }
            
            // 두 번째 열: 파라미터 입력
            ImGui::TableNextColumn();
            
            // 격자 유형별 파라미터 설정 UI (두 번째 열 내에서 또 다른 테이블로 구성)
            switch (i) {
                case 0: // Simple Cubic
                    if (ImGui::BeginTable("SCParams", 3, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        // 0.0f, 0.0f로 step 매개변수 설정하여 +/- 버튼 제거
                        if (ImGui::InputFloat("##a_sc", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b = c, all angles = 90°)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 1: // Body-Centered Cubic
                    if (ImGui::BeginTable("BCCParams", 3, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_bcc", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b = c, all angles = 90°)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 2: // Face-Centered Cubic
                    if (ImGui::BeginTable("FCCParams", 3, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_fcc", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b = c, all angles = 90°)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 3: // Simple Tetragonal
                    if (ImGui::BeginTable("STParams", 4, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_st", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("c:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##c_st", &bravaisParams[i].c, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b != c, all angles = 90°)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 4: // Body-Centered Tetragonal
                    if (ImGui::BeginTable("BCTParams", 4, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_bct", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("c:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##c_bct", &bravaisParams[i].c, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b != c, all angles = 90°)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 5: // Simple Orthorhombic
                case 6: // Body-Centered Orthorhombic
                case 7: // Face-Centered Orthorhombic
                case 8: // Base-Centered Orthorhombic
                    {
                        const char* tableID = i == 5 ? "SOParams" : 
                                             i == 6 ? "BCOParams" : 
                                             i == 7 ? "FCOParams" : "BCOCParams";
                        if (ImGui::BeginTable(tableID, 5, ImGuiTableFlags_SizingFixedFit)) {
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("a:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##a_" + std::to_string(i)).c_str(), &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("b:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##b_" + std::to_string(i)).c_str(), &bravaisParams[i].b, 0.0f, 0.0f, "%.3f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("c:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##c_" + std::to_string(i)).c_str(), &bravaisParams[i].c, 0.0f, 0.0f, "%.3f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::TextDisabled(""); // 빈 공간
                            
                            ImGui::TableNextColumn();
                            ImGui::TextDisabled("(all angles = 90°)");
                            ImGui::EndTable();
                        }
                    }
                    break;
                    
                case 9: // Simple Monoclinic
                case 10: // Base-Centered Monoclinic
                    {
                        const char* tableID = i == 9 ? "SMParams" : "BCMParams";
                        if (ImGui::BeginTable(tableID, 5, ImGuiTableFlags_SizingFixedFit)) {
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("a:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##a_" + std::to_string(i)).c_str(), &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("b:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##b_" + std::to_string(i)).c_str(), &bravaisParams[i].b, 0.0f, 0.0f, "%.3f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("c:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##c_" + std::to_string(i)).c_str(), &bravaisParams[i].c, 0.0f, 0.0f, "%.3f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("beta:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(inputWidth);
                            if (ImGui::InputFloat(("##beta_" + std::to_string(i)).c_str(), &bravaisParams[i].beta, 0.0f, 0.0f, "%.1f")) {
                                if (selectedBravaisType == i) setBravaisLattice(i);
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::TextDisabled("(alpha = gamma = 90°)");
                            ImGui::EndTable();
                        }
                    }
                    break;
                    
                case 11: // Triclinic
                    // 삼사정계는 모든 각도와 변의 길이가 다르므로 두 줄에 나눠서 표시
                    if (ImGui::BeginTable("TriclinicParams1", 4, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_tri", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("b:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##b_tri", &bravaisParams[i].b, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("c:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##c_tri", &bravaisParams[i].c, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("");
                        ImGui::EndTable();
                    }
                    
                    if (ImGui::BeginTable("TriclinicParams2", 4, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("alpha:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##alpha_tri", &bravaisParams[i].alpha, 0.0f, 0.0f, "%.1f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("beta:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##beta_tri", &bravaisParams[i].beta, 0.0f, 0.0f, "%.1f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("gamma:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##gamma_tri", &bravaisParams[i].gamma, 0.0f, 0.0f, "%.1f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(all angles != 90°)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 12: // Rhombohedral
                    if (ImGui::BeginTable("RhombohedralParams", 4, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_rho", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("alpha:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##alpha_rho", &bravaisParams[i].alpha, 0.0f, 0.0f, "%.1f")) {
                            // 삼방정계는 모든 각도가 같음
                            bravaisParams[i].beta = bravaisParams[i].gamma = bravaisParams[i].alpha;
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b = c, alpha = beta = gamma)");
                        ImGui::EndTable();
                    }
                    break;
                    
                case 13: // Hexagonal
                    if (ImGui::BeginTable("HexagonalParams", 4, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("a:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##a_hex", &bravaisParams[i].a, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("c:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        if (ImGui::InputFloat("##c_hex", &bravaisParams[i].c, 0.0f, 0.0f, "%.3f")) {
                            if (selectedBravaisType == i) setBravaisLattice(i);
                        }
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled(""); // 빈 공간
                        
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(a = b, alpha = beta = 90°, gamma = 120°)");
                        ImGui::EndTable();
                    }
                    break;
            }
        }
        ImGui::EndTable();
    }
    
    // 선택된 Bravais 격자 유형에 대한 추가 정보
    if (selectedBravaisType >= 0) {
        ImGui::Separator();
        ImGui::Text("Selected: %s", latticeNames[selectedBravaisType]);
        
        // 선택된 격자의 세부 정보 표시 (옵션)
        if (ImGui::TreeNode("Lattice Description")) {
            // 각 격자 유형에 대한 설명 추가
            switch (selectedBravaisType) {
                case 0: // Simple Cubic
                    ImGui::TextWrapped("Simple cubic has identical lattice parameters (a = b = c) and all angles are 90°. Examples include NaCl, CsCl.");
                    break;
                case 1: // Body-Centered Cubic
                    ImGui::TextWrapped("Body-centered cubic has identical lattice parameters with atoms at each corner and one at the center of the cube. Examples include Fe, Cr, W.");
                    break;

                // TODO : 다른 격자 유형에 대한 설명 추가

                default:
                    ImGui::TextWrapped("Select a Bravais lattice to see its description.");
                    break;
            }
            ImGui::TreePop();
        }
    }
}
