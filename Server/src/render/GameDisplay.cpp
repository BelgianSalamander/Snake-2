//
// Created by Anatol on 24/06/2022.
//

#include <sstream>
#include "render/GameDisplay.h"

#include "imgui.h"

Color BACKGROUND = {26, 26, 26};
Color FOOD = {255, 255, 0};

float margin = 4.0f;

ImVec2 sub(ImVec2 a, ImVec2 b) {
    return {a.x - b.x, a.y - b.y};
}

ImVec2 add(ImVec2 a, ImVec2 b) {
    return {a.x + b.x, a.y + b.y};
}

void drawFilledRect(ImDrawList* l, float fromX, float fromY, float width, float height, Color color) {
    if (width < 0) {
        fromX += width;
        width = -width;
    }

    if (height < 0) {
        fromY += height;
        height = -height;
    }

    l->AddRectFilled(ImVec2(fromX, fromY), ImVec2(fromX + width, fromY + height), (uint32_t) color, 0.0f);
}

//TODO: Show the players
void GameDisplay::renderWindow() {
    bool open;

    if(!ImGui::Begin(
            windowName.c_str(),
            &open
    ) || this->game == nullptr) {
        ImGui::End();
        return;
    }

    ImVec2 drawRegionSize = sub(ImGui::GetWindowContentRegionMax(), ImGui::GetWindowContentRegionMin());
    ImVec2 base = add(ImGui::GetWindowContentRegionMin(), ImGui::GetWindowPos());

    float gridWidth, gridHeight;
    float drawX, drawY;

    float playersBaseX = base.x, playersBaseY = base.y;
    float playersWidth, playersHeight;
    bool playersHorizontal;

    if (drawRegionSize.x * game->getNumRows() > drawRegionSize.y * game->getNumCols()) {
        gridWidth = drawRegionSize.y / game->getNumCols() * game->getNumRows();
        gridHeight = drawRegionSize.y;
        drawX = base.x + (drawRegionSize.x - gridWidth);
        drawY = base.y;

        playersWidth = drawRegionSize.x - gridWidth;
        playersHeight = drawRegionSize.y;
        playersHorizontal = true;
    } else {
        gridWidth = drawRegionSize.x;
        gridHeight = drawRegionSize.x / game->getNumRows() * game->getNumCols();
        drawX = base.x;
        drawY = base.y + (drawRegionSize.y - gridHeight);

        playersWidth = drawRegionSize.x;
        playersHeight = drawRegionSize.y - gridHeight;
        playersHorizontal = false;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawPlayers(drawList, playersBaseX, playersBaseY, playersWidth, playersHeight, playersHorizontal);

    drawX += 4.f;
    drawY += 4.f;

    gridHeight -= 8.f;
    gridWidth -= 8.f;

    drawList->AddRectFilled({drawX, drawY}, {drawX + gridWidth, drawY + gridHeight}, (uint32_t) BACKGROUND, 0.f);

    float gridBaseDrawX = drawX + margin;
    float gridBaseDrawY = drawY + margin;

    float gridCellWidthWithMargin = (gridWidth - margin) / game->getNumCols();
    float gridCellHeightWithMargin = (gridHeight - margin) / game->getNumRows();

    float gridCellWidth = gridCellWidthWithMargin - margin;
    float gridCellHeight = gridCellHeightWithMargin - margin;

    auto rowToGridY = [](const unsigned int row) {
        return row;
    };

    //Draw squares
    for (unsigned int i = 0; i < game->getNumRows(); i++) {
        for (unsigned int j = 0; j < game->getNumCols(); j++) {
            Square square = game->getSquare({i, j});

            if (square.type == SquareType::EMPTY) continue;

            Color color = FOOD;
            if (square.type == SquareType::SNAKE) {
                color = game->snakes[square.snakeID].getPlayer()->getColor();
            }

            drawFilledRect(
                    drawList,
                    gridBaseDrawX + j * gridCellWidthWithMargin,
                    gridBaseDrawY + rowToGridY(i) * gridCellHeightWithMargin,
                    gridCellWidth,
                    gridCellHeight,
                    color
            );
        }
    }

    //Draw snakes
    for (auto snake : game->snakes) {
        if (!snake.isAlive()) continue;

        Player *player = snake.getPlayer();
        Color color = player->getColor();
        std::queue<Pos> bodyRef = snake.getBody();
        std::queue<Pos> body = bodyRef;

        std::vector<Pos> bodyVec;

        while (!body.empty()) {
            bodyVec.push_back(body.front());
            body.pop();
        }

        for (unsigned int j = 0; j < bodyVec.size() - 1; j++) {
            Pos from = bodyVec[j];
            Pos to = bodyVec[j + 1];

            //Draw between the squares from and to, in the margin
            switch (getMove(from, to)) {
                case UP:
                    drawFilledRect(
                            drawList,
                            gridBaseDrawX + from.col * gridCellWidthWithMargin,
                            gridBaseDrawY + rowToGridY(from.row) * gridCellHeightWithMargin,
                            gridCellWidth,
                            -margin,
                            color
                    );
                    break;
                case DOWN:
                    drawFilledRect(
                            drawList,
                            gridBaseDrawX + from.col * gridCellWidthWithMargin,
                            gridBaseDrawY + rowToGridY(from.row) * gridCellHeightWithMargin + gridCellHeight,
                            gridCellWidth,
                            margin,
                            color
                    );
                    break;
                case LEFT:
                    drawFilledRect(
                            drawList,
                            gridBaseDrawX + to.col * gridCellWidthWithMargin + gridCellWidth,
                            gridBaseDrawY + rowToGridY(from.row) * gridCellHeightWithMargin,
                            margin,
                            gridCellHeight,
                            color
                    );
                    break;
                case RIGHT:
                    drawFilledRect(
                            drawList,
                            gridBaseDrawX + to.col * gridCellWidthWithMargin,
                            gridBaseDrawY + rowToGridY(from.row) * gridCellHeightWithMargin,
                            -margin,
                            gridCellHeight,
                            color
                    );
                    break;
            }
        }

        Move headDirection;

        if (bodyVec.size() > 1) {
            headDirection = getMove(bodyVec[bodyVec.size() - 2], bodyVec.back());
        } else {
            headDirection = UP;
        }

        float headCenterX = gridBaseDrawX + bodyVec.back().col * gridCellWidthWithMargin + gridCellWidth / 2;
        float headCenterY = gridBaseDrawY + rowToGridY(bodyVec.back().row) * gridCellHeightWithMargin + gridCellHeight / 2;

        float eyeBaseX = headCenterX + 0.3f * MOVES[headDirection].second * gridCellWidth;
        float eyeBaseY = headCenterY + 0.3f * MOVES[headDirection].first * gridCellHeight;

        auto p = perp(headDirection);

        drawList->AddCircleFilled(
                ImVec2(
                        eyeBaseX + 0.2f * MOVES[p.first].second * gridCellWidth,
                        eyeBaseY + 0.2f * MOVES[p.first].first * gridCellHeight
                ),
                0.15f * gridCellWidth,
                0xFF000000
        );

        drawList->AddCircleFilled(
                ImVec2(
                        eyeBaseX + 0.2f * MOVES[p.second].second * gridCellWidth,
                        eyeBaseY + 0.2f * MOVES[p.second].first * gridCellHeight
                ),
                0.15f * gridCellWidth,
                0xFF000000
        );
    }

    ImGui::End();
}

GameDisplay::GameDisplay(std::string name) {
    this->windowName = name;
}

void GameDisplay::drawPlayers(ImDrawList *pList, float x, float y, float width, float height, bool horizontal) {
    ImGui::BeginChild(windowName.c_str(), ImVec2(width, height), true);

    std::vector<Snake*> snakes;

    for (Snake& snake: game->snakes) {
        snakes.push_back(&snake);
    }

    std::sort(snakes.begin(), snakes.end(), [](Snake *a, Snake *b) {
        if (a->isAlive() != b->isAlive()) {
            return a->isAlive() > b->isAlive();
        } else {
            return a->getBody().size() > b->getBody().size();
        }
    });


    for (auto snake: snakes) {
        Player *player = snake->getPlayer();
        Color color = player->getColor();

        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, (uint32_t) color);
        ImGui::Text("%s%s: %llu", player->getName().c_str(), snake->isAlive() ? "" : player->kicked ? " (timeout)" : " (dead)", snake->getBody().size());
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}
