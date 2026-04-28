// 주기율표 렌더링 - ImGui 스타일 스택 에러 수정 버전
void AtomsTemplate::renderPeriodicTable() {
    // 선택된 원소를 저장할 인덱스 변수
    static int selectedElement = -1;
    static int category = 0;  // 0: 모든 원소, 1: 금속, 2: 비금속, 3: 준금속, 4: 불활성 기체
    
    // 원자 위치 입력을 위한 변수
    static float atomPosition[3] = {0.0f, 0.0f, 0.0f};
    
    // 원소 분류 선택
    if (ImGui::BeginCombo(
        "Classification", 
        category == 0 ? "All Elements" : 
        category == 1 ? "Non-metals" : 
        category == 2 ? "Alkali Metals" : 
        category == 3 ? "Alkaline Earth Metals" :
        category == 4 ? "Transition Metals" :
        category == 5 ? "Post-transition Metals" :
        category == 6 ? "Metalloid" :
        category == 7 ? "Halogens" : 
        category == 8 ? "Noble Gases" :
        category == 9 ? "Lanthanide" : "Actinide")) {
        if (ImGui::Selectable("All Elements",  category == 0)) category = 0;
        if (ImGui::Selectable("Non-metals",    category == 1)) category = 1;
        if (ImGui::Selectable("Alkali Metals", category == 2)) category = 2;
        if (ImGui::Selectable("Alkaline Earth Metals",  category == 3)) category = 3;
        if (ImGui::Selectable("Transition Metals",      category == 4)) category = 4;
        if (ImGui::Selectable("Post-transition Metals", category == 5)) category = 5;
        if (ImGui::Selectable("Metalloid",   category == 6)) category = 6;
        if (ImGui::Selectable("Halogens",    category == 7)) category = 7;
        if (ImGui::Selectable("Noble Gases", category == 8)) category = 8;
        if (ImGui::Selectable("Lanthanide",  category == 9)) category = 9;
        if (ImGui::Selectable("Actinide",    category == 10)) category = 10;
        ImGui::EndCombo();
    }

    // 버튼 크기와 간격 설정
    const float buttonSize = 40.0f;
    const float spacing = 2.0f;
    const float totalWidth = (buttonSize + spacing) * 18 + spacing; // 18개 족
    
    // 창 너비에 맞게 스케일 조정
    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float scale = availWidth / totalWidth;
    const float scaledButtonSize = buttonSize * scale;
    const float scaledSpacing = spacing * scale;

    // 스타일 설정
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(scaledSpacing, scaledSpacing));
    
    // 주기율표 상단 설명 추가
    ImGui::TextWrapped("Click on an element to select it. Selected element: %s", selectedElement >= 0 ? elements[selectedElement].name : "None");
    ImGui::Separator();

    // 메인 주기율표 표시 (1-7주기, 18족)
    for (int period = 1; period <= 7; period++) {
        for (int group = 1; group <= 18; group++) {

            // 해당 위치에 원소가 있는지 확인
            bool hasElement = false;
            int elementIndex = -1;
            for (size_t i = 0; i < elements.size(); i++) {
                if (elements[i].period == period && elements[i].group == group) {
                    hasElement = true;
                    elementIndex = static_cast<int>(i);
                    break;
                }
            }
            
            // 이 위치에 원소가 있는 경우에만 버튼 렌더링
            if (hasElement) {
                // 같은 줄에 여러 버튼을 나란히 배치
                if (group > 1) {
                    ImGui::SameLine();
                }
                
                // 원소 필터링 (카테고리에 따라) 
                const Element& element = elements[elementIndex];
                bool isVisible = true;
  
                if (category > 0) {
                    switch (category) {
                        case 1: // Non-metals
                            isVisible = (element.Z == 1) || 
                                       (element.Z >= 6 && element.Z <= 9) ||
                                       (element.Z >= 15 && element.Z <= 17) ||
                                       (element.Z == 34 || element.Z == 35) ||
                                       (element.Z == 53) || (element.Z == 85);
                            break;
                            
                        case 2: // Alkali Metals
                            isVisible = (element.Z == 3) || (element.Z == 11) || 
                                       (element.Z == 19) || (element.Z == 37) || 
                                       (element.Z == 55) || (element.Z == 87);
                            break;
                            
                        case 3: // Alkaline Earth Metals
                            isVisible = (element.Z == 4) || (element.Z == 12) || 
                                       (element.Z == 20) || (element.Z == 38) || 
                                       (element.Z == 56) || (element.Z == 88);
                            break;
                            
                        case 4: // Transition Metals
                            isVisible = (element.Z >= 21 && element.Z <= 30) ||
                                       (element.Z >= 39 && element.Z <= 48) ||
                                       (element.Z >= 72 && element.Z <= 80) ||
                                       (element.Z >= 104 && element.Z <= 112);
                            break;
                            
                        case 5: // Post-transition Metals
                            isVisible = (element.Z == 13) || (element.Z == 31) ||
                                       (element.Z == 49 || element.Z == 50) ||
                                       (element.Z >= 81 && element.Z <= 83) ||
                                       (element.Z >= 113 && element.Z <= 116);
                            break;
                            
                        case 6: // Metalloids
                            isVisible = (element.Z == 5) || (element.Z == 14) ||
                                       (element.Z == 32 || element.Z == 33) ||
                                       (element.Z == 51 || element.Z == 52) ||
                                       (element.Z == 84);
                            break;
                            
                        case 7: // Halogens
                            isVisible = (element.Z == 9) || (element.Z == 17) ||
                                       (element.Z == 35) || (element.Z == 53) ||
                                       (element.Z == 85) || (element.Z == 117);
                            break;
                            
                        case 8: // Noble Gases
                            isVisible = (element.Z == 2) || (element.Z == 10) ||
                                       (element.Z == 18) || (element.Z == 36) ||
                                       (element.Z == 54) || (element.Z == 86) ||
                                       (element.Z == 118);
                            break;
                            
                        case 9: // Lanthanides
                            isVisible = (element.Z >= 57 && element.Z <= 71);
                            break;
                            
                        case 10: // Actinides
                            isVisible = (element.Z >= 89 && element.Z <= 103);
                            break;
                            
                        default:
                            isVisible = true;
                            break;
                    }
                }

                if (isVisible) {
                    // ✅ 수정: 정확한 Push/Pop 카운팅
                    int pushedColors = 0;
                    int pushedVars = 0;
                    
                    // 호버 및 액티브 색상 계산
                    ImVec4 hoveredColor = ImVec4(
                        element.color.x * 1.2f,
                        element.color.y * 1.2f,
                        element.color.z * 1.2f,
                        element.color.w
                    );
                    ImVec4 activeColor = ImVec4(
                        element.color.x * 0.8f,
                        element.color.y * 0.8f,
                        element.color.z * 0.8f,
                        element.color.w
                    );
                    ImVec4 textColor = getContrastTextColor(element.color);
                    
                    // 원소 버튼 스타일 설정
                    ImGui::PushStyleColor(ImGuiCol_Button, element.color);
                    pushedColors++;
                    
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                    pushedColors++;
                    
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                    pushedColors++;
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                    pushedColors++;
                    
                    // 선택된 원소는 테두리로 표시
                    if (elementIndex == selectedElement) {
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        pushedColors++;
                        
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                        pushedVars++;
                    }
                    
                    // 버튼 생성
                    char buttonLabel[16];
                    snprintf(buttonLabel, sizeof(buttonLabel), "%s\n%d", element.symbol, element.Z);
                    
                    if (ImGui::Button(buttonLabel, ImVec2(scaledButtonSize, scaledButtonSize))) {
                        selectedElement = elementIndex;
                    }
                    
                    // 마우스 호버 시 툴팁 표시
                    if (ImGui::IsItemHovered()) {
                        // ✅ 툴팁용 별도 스타일 관리 (독립적인 Push/Pop)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        
                        ImGui::BeginTooltip();
                        ImGui::Text("%s (%s)", element.name, element.symbol);
                        ImGui::Text("Atomic Number: %d", element.Z);
                        ImGui::Text("Atomic Mass: %.4f", element.mass);
                        ImGui::Text("Covalent Radius: %.2f Å", element.radius);
                        ImGui::Text("Group: %d, Period: %d", element.group, element.period);
                        ImGui::Text("Classification: %s", element_classifications[element.Z]);
                        ImGui::EndTooltip();
                        
                        ImGui::PopStyleColor(); // 툴팁용 색상만 Pop (1개)
                    }
                    
                    // ✅ 정확한 개수로 Pop (Push한 만큼만)
                    if (pushedColors > 0) {
                        ImGui::PopStyleColor(pushedColors);
                    }
                    if (pushedVars > 0) {
                        ImGui::PopStyleVar(pushedVars);
                    }
                }
                else {
                    // 보이지 않는 원소는 투명한 더미로 대체
                    if (group > 1) {
                        ImGui::SameLine();
                    }
                    ImGui::Dummy(ImVec2(scaledButtonSize, scaledButtonSize));
                }
            }
            else if ((period <= 2 && group >= 3 && group <= 12) || 
                     (period == 1 && group >= 13 && group <= 17)) {
                // 빈 공간 (원소가 없는 위치)
                if (group > 1) {
                    ImGui::SameLine();
                }
                // 빈 공간에 투명한 버튼 배치
                ImGui::Dummy(ImVec2(scaledButtonSize, scaledButtonSize));
            }
            else {
                // 일반적인 위치의 원소 (빈 공간이 아님)
                if (group > 1) {
                    ImGui::SameLine();
                }
                ImGui::Dummy(ImVec2(scaledButtonSize, scaledButtonSize));
            }
        }
    }
    
    // 란타넘족과 악티늄족을 위한 여백
    ImGui::Dummy(ImVec2(1.0f, 10.0f));
    
    // 란타넘족 표시 (8주기, 3-17족)
    if (category == 0 || category == 9) {
        ImGui::Text("Lanthanides:");
        for (int group = 3; group <= 17; group++) {
            // 해당 위치에 원소가 있는지 확인
            bool hasElement = false;
            int elementIndex = -1;
            
            for (size_t i = 0; i < elements.size(); i++) {
                if (elements[i].period == 8 && elements[i].group == group) {
                    hasElement = true;
                    elementIndex = static_cast<int>(i);
                    break;
                }
            }
            
            // 란타넘족 원소 표시
            if (hasElement) {
                if (group > 3) {
                    ImGui::SameLine();
                }
                
                const Element& element = elements[elementIndex];
                
                // ✅ 수정: 정확한 Push/Pop 카운팅 (란타넘족)
                int pushedColors = 0;
                int pushedVars = 0;
                
                // 호버 및 액티브 색상 계산
                ImVec4 hoveredColor = ImVec4(
                    element.color.x * 1.2f,
                    element.color.y * 1.2f,
                    element.color.z * 1.2f,
                    element.color.w
                );
                ImVec4 activeColor = ImVec4(
                    element.color.x * 0.8f,
                    element.color.y * 0.8f,
                    element.color.z * 0.8f,
                    element.color.w
                );
                ImVec4 textColor = getContrastTextColor(element.color);
                
                // 원소 버튼 스타일 설정
                ImGui::PushStyleColor(ImGuiCol_Button, element.color);
                pushedColors++;
                
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                pushedColors++;
                
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                pushedColors++;
                
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                pushedColors++;
                
                // 선택된 원소는 테두리로 표시
                if (elementIndex == selectedElement) {
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    pushedColors++;
                    
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                    pushedVars++;
                }
                
                // 버튼 생성
                char buttonLabel[16];
                snprintf(buttonLabel, sizeof(buttonLabel), "%s\n%d", element.symbol, element.Z);
                
                if (ImGui::Button(buttonLabel, ImVec2(scaledButtonSize, scaledButtonSize))) {
                    selectedElement = elementIndex;
                }
                
                // 마우스 호버 시 툴팁 표시
                if (ImGui::IsItemHovered()) {
                    // ✅ 툴팁용 별도 스타일 관리
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    
                    ImGui::BeginTooltip();
                    ImGui::Text("%s (%s)", element.name, element.symbol);
                    ImGui::Text("Atomic Number: %d", element.Z);
                    ImGui::Text("Atomic Mass: %.4f", element.mass);
                    ImGui::Text("Covalent Radius: %.2f Å", element.radius);
                    ImGui::Text("Classification: %s", element_classifications[element.Z]);
                    ImGui::EndTooltip();
                    
                    ImGui::PopStyleColor(); // 툴팁용 색상만 Pop
                }
                
                // ✅ 정확한 개수로 Pop
                if (pushedColors > 0) {
                    ImGui::PopStyleColor(pushedColors);
                }
                if (pushedVars > 0) {
                    ImGui::PopStyleVar(pushedVars);
                }
            }
        }
    }
    
    // 악티늄족 표시 (9주기, 3-17족)
    if (category == 0 || category == 10) {
        ImGui::Text("Actinides:");
        for (int group = 3; group <= 17; group++) {
            // 해당 위치에 원소가 있는지 확인
            bool hasElement = false;
            int elementIndex = -1;
            
            for (size_t i = 0; i < elements.size(); i++) {
                if (elements[i].period == 9 && elements[i].group == group) {
                    hasElement = true;
                    elementIndex = static_cast<int>(i);
                    break;
                }
            }
            
            // 악티늄족 원소 표시
            if (hasElement) {
                if (group > 3) {
                    ImGui::SameLine();
                }
                
                const Element& element = elements[elementIndex];
                
                // ✅ 수정: 정확한 Push/Pop 카운팅 (악티늄족)
                int pushedColors = 0;
                int pushedVars = 0;
                
                // 호버 및 액티브 색상 계산
                ImVec4 hoveredColor = ImVec4(
                    element.color.x * 1.2f,
                    element.color.y * 1.2f,
                    element.color.z * 1.2f,
                    element.color.w
                );
                ImVec4 activeColor = ImVec4(
                    element.color.x * 0.8f,
                    element.color.y * 0.8f,
                    element.color.z * 0.8f,
                    element.color.w
                );
                ImVec4 textColor = getContrastTextColor(element.color);
                
                // 원소 버튼 스타일 설정
                ImGui::PushStyleColor(ImGuiCol_Button, element.color);
                pushedColors++;
                
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                pushedColors++;
                
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                pushedColors++;
                
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                pushedColors++;
                
                // 선택된 원소는 테두리로 표시
                if (elementIndex == selectedElement) {
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    pushedColors++;
                    
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                    pushedVars++;
                }
                
                // 버튼 생성
                char buttonLabel[16];
                snprintf(buttonLabel, sizeof(buttonLabel), "%s\n%d", element.symbol, element.Z);
                
                if (ImGui::Button(buttonLabel, ImVec2(scaledButtonSize, scaledButtonSize))) {
                    selectedElement = elementIndex;
                }
                
                // 마우스 호버 시 툴팁 표시
                if (ImGui::IsItemHovered()) {
                    // ✅ 툴팁용 별도 스타일 관리
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    
                    ImGui::BeginTooltip();
                    ImGui::Text("%s (%s)", element.name, element.symbol);
                    ImGui::Text("Atomic Number: %d", element.Z);
                    ImGui::Text("Atomic Mass: %.4f", element.mass);
                    ImGui::Text("Covalent Radius: %.2f Å", element.radius);
                    ImGui::Text("Classification: %s", element_classifications[element.Z]);
                    ImGui::EndTooltip();
                    
                    ImGui::PopStyleColor(); // 툴팁용 색상만 Pop
                }
                
                // ✅ 정확한 개수로 Pop
                if (pushedColors > 0) {
                    ImGui::PopStyleColor(pushedColors);
                }
                if (pushedVars > 0) {
                    ImGui::PopStyleVar(pushedVars);
                }
            }
        }
    }

    // 스타일 리셋 (함수 시작에서 Push한 ItemSpacing)
    ImGui::PopStyleVar(); // ItemSpacing
    
    // 선택된 원소에 대한 상세 정보
    if (selectedElement >= 0 && selectedElement < static_cast<int>(elements.size())) {
        const Element& element = elements[selectedElement];
        
        // 원자 위치 입력
        ImGui::Separator();
        ImGui::Text("Atom Position");

        // Cell이 없는 경우 체크박스 비활성화
        bool hasCellInfo = !cellEdgeActors.empty();

        if (!hasCellInfo) {
            ImGui::BeginDisabled(); // 비활성화 시작
        }

        // 분율 좌표 사용 여부 체크박스 추가
        static bool useFractionalCoords = false;
        ImGui::Checkbox("Use Fractional Coordinates##PeriodicTable", &useFractionalCoords);
        
        // 체크박스가 비활성화된 경우에도 툴팁을 표시하기 위해 ImGuiHoveredFlags_AllowWhenDisabled 플래그 사용
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasCellInfo) {
            ImGui::BeginTooltip();
            ImGui::Text("Fractional coordinates require a unit cell.\nPlease create or import a cell first.");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Fractional coordinates are relative to the unit cell axes (values from 0 to 1)");
            ImGui::EndTooltip();
        }
        
        if (!hasCellInfo) {
            ImGui::EndDisabled(); // 비활성화 종료
            useFractionalCoords = false; // Cell이 없는 경우 강제로 분율 좌표 모드 비활성화
        }

        // 직교/분율 좌표에 따라 다른 라벨 표시
        if (useFractionalCoords && hasCellInfo) {
            static float fracPosition[3] = {0.0f, 0.0f, 0.0f};
            
            // Fractional 좌표 입력
            ImGui::DragFloat3("Fractional (a,b,c)", fracPosition, 0.01f, 0.0f, 1.0f);
            
            // 선택된 원소로 할 수 있는 액션 버튼
            if (ImGui::Button("Add to Structure")) {
                // Fractional 좌표를 Cartesian 좌표로 변환
                float cartPosition[3];
                fractionalToCartesian(fracPosition, cartPosition, cellInfo.matrix);
                
                // 선택된 원소를 기반으로 구체 생성 (Cartesian 좌표 사용)
                createAtomSphere(element.symbol, element.color, element.radius, cartPosition);
            }
        } 
        else {
            // 직교 좌표 입력 (기존 코드)
            ImGui::DragFloat3("Position (X,Y,Z)", atomPosition, 0.1f);
            
            // 선택된 원소로 할 수 있는 액션 버튼
            if (ImGui::Button("Add to Structure")) {
                // 선택된 원소를 기반으로 구체 생성
                createAtomSphere(element.symbol, element.color, element.radius, atomPosition);
            }
        }
    }
}
