// SPDX-License-Identifier: MIT
// Copyright (c) 2016 Hotride
// Unmodified function from crossuo/crossuo@79d0f94a3645154dc1795045d9d89df554d6cd2a
// src/Walker/PathFinder.cpp
bool CPathFinder::CalculateNewZ(int x, int y, char &z, int direction)
{
    int stepState = PSS_NORMAL;

    if (g_Player->IsDead() || g_Player->Graphic == 0x03DB)
    {
        stepState = PSS_DEAD_OR_GM;
    }
    else
    {
        if (g_Player->IsFlying())
        { // && no mount?
            stepState = PSS_FLYING;
        }
        else
        {
            CGameItem *mount = g_Player->FindLayer(OL_MOUNT);

            if (mount != nullptr && mount->Graphic == 0x3EB3)
            { //Sea horse
                stepState = PSS_ON_SEA_HORSE;
            }
        }
    }

    int minZ = -128;
    int maxZ = z;

    CalculateMinMaxZ(minZ, maxZ, x, y, z, direction, stepState);

    std::vector<CPathObject> list;

    if (g_CustomHouseGump != nullptr)
    {
        SDL_Rect rect = { g_CustomHouseGump->StartPos.X,
                          g_CustomHouseGump->StartPos.Y,
                          g_CustomHouseGump->EndPos.X,
                          g_CustomHouseGump->EndPos.Y };
        SDL_Point pos = { x, y };

        if (SDL_PointInRect(&pos, &rect) == 0u)
        {
            return false;
        }
    }

    if (!CreateItemsList(list, x, y, stepState) || list.empty())
    {
        return false;
    }

    auto compareFunction = [](const void *obj1, const void *obj2) {
        int result = 0;

        if (obj1 != nullptr && obj2 != nullptr)
        {
            result = ((CPathObject *)obj1)->Z - ((CPathObject *)obj2)->Z;

            if (result == 0)
            {
                result = (((CPathObject *)obj1)->Height - ((CPathObject *)obj2)->Height);
            }
        }

        return result;
    };

    std::qsort(&list[0], list.size(), sizeof(CPathObject), compareFunction);

    list.push_back(CPathObject(POF_IMPASSABLE_OR_SURFACE, 128, 128, 128, nullptr));

    int resultZ = -128;

    if (z < minZ)
    {
        z = (char)minZ;
    }

    int currentTempObjZ = 1000000;
    int currentZ = -128;

    int listSize = (int)list.size();

    for (int i = 0; i < listSize; i++)
    {
        const CPathObject &obj = list[i];

        if (((obj.Flags & POF_NO_DIAGONAL) != 0u) && stepState == PSS_FLYING)
        {
            int objAverageZ = obj.AverageZ;

            int delta = abs(objAverageZ - (int)z);

            if (delta <= 25)
            {
                if (objAverageZ != -128)
                {
                    resultZ = objAverageZ;
                }
                else
                {
                    resultZ = currentZ;
                }

                break;
            }
        }

        if ((obj.Flags & POF_IMPASSABLE_OR_SURFACE) != 0u)
        {
            int objZ = obj.Z;

            if (objZ - minZ >= DEFAULT_BLOCK_HEIGHT)
            {
                for (int j = i - 1; j >= 0; j--)
                {
                    const CPathObject &tempObj = list[j];

                    if ((tempObj.Flags & (POF_SURFACE | POF_BRIDGE)) != 0u)
                    {
                        int tempAverageZ = tempObj.AverageZ;

                        if (tempAverageZ >= currentZ &&
                            objZ - tempAverageZ >= DEFAULT_BLOCK_HEIGHT &&
                            ((tempAverageZ <= maxZ && ((tempObj.Flags & POF_SURFACE) != 0u)) ||
                             (((tempObj.Flags & POF_BRIDGE) != 0u) && tempObj.Z <= maxZ)))
                        {
                            int delta = abs((int)z - tempAverageZ);

                            if (delta < currentTempObjZ)
                            {
                                currentTempObjZ = delta;
                                resultZ = tempAverageZ;
                            }
                        }
                    }
                }
            }

            if (g_ConfigManager.AutoOpenDoors && obj.m_Object && obj.m_Object->IsDoor())
            {
                g_Game.OpenDoor();
            }

            const int averageZ = obj.AverageZ;
            if (minZ < averageZ)
            {
                minZ = averageZ;
            }

            if (currentZ < averageZ)
            {
                currentZ = averageZ;
            }
        }
    }
    z = (char)resultZ;
    return (resultZ != -128);
}
