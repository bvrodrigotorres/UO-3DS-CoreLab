// SPDX-License-Identifier: MIT
// Copyright (c) 2016 Hotride
// Unmodified function from crossuo/crossuo@79d0f94a3645154dc1795045d9d89df554d6cd2a
// src/GameObjects/MapBlock.cpp
void CMapBlock::AddRender(CRenderWorldObject *item, int x, int y)
{
    item->RemoveRender();
    int priorityZ = item->GetZ();
    if (item->IsLandObject())
    {
        if (((CLandObject *)item)->IsStretched)
        {
            priorityZ = ((CLandObject *)item)->AverageZ - 1;
        }
        else
        {
            priorityZ--;
        }
    }
    else if (item->IsStaticGroupObject())
    {
        if (item->IsGameObject())
        {
            //priorityZ++;
            if (((CGameObject *)item)->NPC || ((CGameObject *)item)->IsCorpse())
            {
                priorityZ++;
            }
            else
            {
                goto process_static;
            }
        }
        else if (
            item->IsMultiObject() &&
            ((((CMultiObject *)item)->State & CHMOF_GENERIC_INTERNAL) != 0))
        {
            priorityZ--;
        }
        else
        {
        process_static:
            if (item->IsBackground())
            {
                priorityZ--;
            }

            if (((CRenderStaticObject *)item)->GetStaticHeight() != 0u)
            {
                priorityZ++;
            }
        }
    }
    else if (item->IsEffectObject())
    {
        priorityZ += 2;
    }

    item->PriorityZ = priorityZ;
    CRenderWorldObject *obj = Block[x][y];
    if (obj == item)
    {
        if (obj->m_Prev != nullptr)
        {
            obj = (CRenderWorldObject *)obj->m_Prev;
        }
        else if (obj->m_Next != nullptr)
        {
            obj = (CRenderWorldObject *)obj->m_Next;
        }
        else
        {
            return;
        }
    }

    while (obj != nullptr && obj->m_PrevXY != nullptr)
    {
        obj = obj->m_PrevXY;
    }

    CRenderWorldObject *found = nullptr;
    CRenderWorldObject *start = obj;
    while (obj != nullptr)
    {
        int testPriorityZ = obj->PriorityZ;

        if (testPriorityZ > priorityZ ||
            (testPriorityZ == priorityZ && item->IsLandObject() && !obj->IsLandObject()))
        {
            break;
        }
        found = obj;
        obj = obj->m_NextXY;
    }

    if (found != nullptr)
    {
        item->m_PrevXY = found;
        CRenderWorldObject *next = found->m_NextXY;
        item->m_NextXY = next;
        found->m_NextXY = item;
        if (next != nullptr)
        {
            next->m_PrevXY = item;
        }
    }
    else if (start != nullptr)
    {
        item->m_NextXY = start;
        start->m_PrevXY = item;
        item->m_PrevXY = nullptr;
    }
}
