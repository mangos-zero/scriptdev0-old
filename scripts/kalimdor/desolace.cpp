/*
 * Copyright (C) 2006-2012 ScriptDev2 <http://www.scriptdev2.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Desolace
SD%Complete: 100
SDComment: Quest support: 1440, 5561, 6132
SDCategory: Desolace
EndScriptData */

/* ContentData
npc_aged_dying_ancient_kodo
npc_dalinda_malem
npc_melizza_brimbuzzle
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_aged_dying_ancient_kodo
######*/

enum
{
    SAY_SMEED_HOME_1                = -1000348,
    SAY_SMEED_HOME_2                = -1000349,
    SAY_SMEED_HOME_3                = -1000350,

    QUEST_KODO                      = 5561,

    NPC_SMEED                       = 11596,
    NPC_AGED_KODO                   = 4700,
    NPC_DYING_KODO                  = 4701,
    NPC_ANCIENT_KODO                = 4702,
    NPC_TAMED_KODO                  = 11627,

    SPELL_KODO_KOMBO_ITEM           = 18153,
    SPELL_KODO_KOMBO_PLAYER_BUFF    = 18172,                //spells here have unclear function, but using them at least for visual parts and checks
    SPELL_KODO_KOMBO_DESPAWN_BUFF   = 18377,
    SPELL_KODO_KOMBO_GOSSIP         = 18362

};

struct MANGOS_DLL_DECL npc_aged_dying_ancient_kodoAI : public ScriptedAI
{
    npc_aged_dying_ancient_kodoAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiDespawnTimer;

    void Reset()
    {
        m_uiDespawnTimer = 0;
    }

    void MoveInLineOfSight(Unit* pWho)
    {
        if (pWho->GetEntry() == NPC_SMEED)
        {
            if (m_creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
                return;

            if (m_creature->IsWithinDistInMap(pWho, 10.0f))
            {
                switch (urand(0, 2))
                {
                    case 0: DoScriptText(SAY_SMEED_HOME_1, pWho); break;
                    case 1: DoScriptText(SAY_SMEED_HOME_2, pWho); break;
                    case 2: DoScriptText(SAY_SMEED_HOME_3, pWho); break;
                }

                //spell have no implemented effect (dummy), so useful to notify spellHit
                m_creature->CastSpell(m_creature, SPELL_KODO_KOMBO_GOSSIP, true);
            }
        }
    }

    void SpellHit(Unit* pCaster, SpellEntry const* pSpell)
    {
        if (pSpell->Id == SPELL_KODO_KOMBO_GOSSIP)
        {
            m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            m_uiDespawnTimer = 60000;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        //timer should always be == 0 unless we already updated entry of creature. Then not expect this updated to ever be in combat.
        if (m_uiDespawnTimer && m_uiDespawnTimer <= diff)
        {
            if (!m_creature->getVictim() && m_creature->isAlive())
            {
                Reset();
                m_creature->SetDeathState(JUST_DIED);
                m_creature->Respawn();
                return;
            }
        }
        else m_uiDespawnTimer -= diff;

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_aged_dying_ancient_kodo(Creature* pCreature)
{
    return new npc_aged_dying_ancient_kodoAI(pCreature);
}

bool EffectDummyCreature_npc_aged_dying_ancient_kodo(Unit *pCaster, uint32 spellId, SpellEffectIndex effIndex, Creature *pCreatureTarget)
{
    //always check spellid and effectindex
    if (spellId == SPELL_KODO_KOMBO_ITEM && effIndex == EFFECT_INDEX_0)
    {
        //no effect if player/creature already have aura from spells
        if (pCaster->HasAura(SPELL_KODO_KOMBO_PLAYER_BUFF) || pCreatureTarget->HasAura(SPELL_KODO_KOMBO_DESPAWN_BUFF))
            return true;

        if (pCreatureTarget->GetEntry() == NPC_AGED_KODO ||
            pCreatureTarget->GetEntry() == NPC_DYING_KODO ||
            pCreatureTarget->GetEntry() == NPC_ANCIENT_KODO)
        {
            pCaster->CastSpell(pCaster, SPELL_KODO_KOMBO_PLAYER_BUFF, true);

            pCreatureTarget->UpdateEntry(NPC_TAMED_KODO);
            pCreatureTarget->CastSpell(pCreatureTarget, SPELL_KODO_KOMBO_DESPAWN_BUFF, false);

            if (pCreatureTarget->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                pCreatureTarget->GetMotionMaster()->MoveIdle();

            pCreatureTarget->GetMotionMaster()->MoveFollow(pCaster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        }

        //always return true when we are handling this spell and effect
        return true;
    }
    return false;
}

bool GossipHello_npc_aged_dying_ancient_kodo(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->HasAura(SPELL_KODO_KOMBO_PLAYER_BUFF) && pCreature->HasAura(SPELL_KODO_KOMBO_DESPAWN_BUFF))
    {
        //the expected quest objective
        pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());

        pPlayer->RemoveAurasDueToSpell(SPELL_KODO_KOMBO_PLAYER_BUFF);
        pCreature->GetMotionMaster()->MoveIdle();
    }

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());
    return true;
}

/*######
## npc_dalinda_malem
######*/

enum
{
    QUEST_RETURN_TO_VAHLARRIEL  = 1440,
};

struct MANGOS_DLL_DECL npc_dalinda_malemAI : public npc_escortAI
{
    npc_dalinda_malemAI(Creature* m_creature) : npc_escortAI(m_creature) { Reset(); }

    void Reset() {}

    void JustStartedEscort()
    {
        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
    }

    void WaypointReached(uint32 uiPointId)
    {
        if (uiPointId == 18)
        {
            if (Player* pPlayer = GetPlayerForEscort())
                pPlayer->GroupEventHappens(QUEST_RETURN_TO_VAHLARRIEL, m_creature);
        }
    }
};

CreatureAI* GetAI_npc_dalinda_malem(Creature* pCreature)
{
    return new npc_dalinda_malemAI(pCreature);
}

bool QuestAccept_npc_dalinda_malem(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_RETURN_TO_VAHLARRIEL)
    {
        if (npc_dalinda_malemAI* pEscortAI = dynamic_cast<npc_dalinda_malemAI*>(pCreature->AI()))
        {
            // TODO This faction change needs confirmation, also possible that we need to drop her PASSIVE flag
            pCreature->setFaction(FACTION_ESCORT_A_NEUTRAL_PASSIVE);
            pEscortAI->Start(false, pPlayer, pQuest);
        }
    }
    return true;
}

/*######
## npc_melizza_brimbuzzle
######*/

enum
{
    QUEST_GET_ME_OUT_OF_HERE   = 6132,

    GO_MELIZZAS_CAGE           = 177706,

    SAY_MELIZZA_START          = -1000710,
    SAY_MELIZZA_FINISH         = -1000711,
    SAY_MELIZZA_1              = -1000712,
    SAY_MELIZZA_2              = -1000713,
    SAY_MELIZZA_3              = -1000714,

    NPC_MARAUDINE_MARAUDER     = 4659,
    NPC_MARAUDINE_WRANGLER     = 4655,
    NPC_MARAUDINE_BONEPAW      = 4660
};

static const float aMarauderSpawn[4][4] =
{
    { -1294.016f, 2637.383f, 111.556f, 1.069f},
    { -1298.763f, 2639.983f, 111.604f, 1.069f},
    { -1312.778f, 2671.861f, 111.908f, 0.107f},
    { -1311.301f, 2677.125f, 111.757f, 0.107f}
};

static const float aWranglerSpawn[3][4] =
{
    { -1387.094f, 2431.129f, 88.829f, 1.472f},
    { -1390.120f, 2429.159f, 88.765f, 1.472f},
    { -1385.852f, 2425.109f, 88.765f, 1.472f}
};

static const float aBonepawSpawn[2][4] =
{
    { -1383.824f, 2434.019f, 88.863f, 1.472f},
    { -1388.103f, 2428.754f, 88.824f, 1.472f}
};

struct MANGOS_DLL_DECL npc_melizza_brimbuzzleAI : public npc_escortAI
{
    npc_melizza_brimbuzzleAI(Creature* m_creature) : npc_escortAI(m_creature)
    {
        m_uiNormalFaction = m_creature->getFaction();
        m_uiEventTimer    = 0;
        m_uiEventPhase    = 0;
        m_bCanDoEvent     = false;
        Reset();
    }

    uint32 m_uiNormalFaction;
    uint32 m_uiEventTimer;

    uint8 m_uiEventPhase;

    bool m_bCanDoEvent;

    void Reset() {}

    void JustStartedEscort()
    {
        if (GameObject* pCage = GetClosestGameObjectWithEntry(m_creature, GO_MELIZZAS_CAGE, INTERACTION_DISTANCE))
            pCage->UseDoorOrButton();
    }

    void WaypointReached(uint32 uiPointId)
    {
        switch (uiPointId)
        {
            case 1:
            {
                if (Player* pPlayer = GetPlayerForEscort())
                    DoScriptText(SAY_MELIZZA_START, m_creature, pPlayer);

                m_creature->setFaction(FACTION_ESCORT_N_NEUTRAL_PASSIVE);
                break;
            }
            case 4:
            {
                for (uint8 i = 0; i < 4; ++i)
                    m_creature->SummonCreature(NPC_MARAUDINE_MARAUDER, aMarauderSpawn[i][0], aMarauderSpawn[i][1], aMarauderSpawn[i][2], aMarauderSpawn[i][3], TEMPSUMMON_DEAD_DESPAWN, 0);

                break;
            }
            case 9:
            {
                for (uint8 i = 0; i < 3; ++i)
                    m_creature->SummonCreature(NPC_MARAUDINE_WRANGLER, aWranglerSpawn[i][0], aWranglerSpawn[i][1], aWranglerSpawn[i][2], aWranglerSpawn[i][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                for (uint8 i = 0; i < 2; ++i)
                    m_creature->SummonCreature(NPC_MARAUDINE_BONEPAW, aBonepawSpawn[i][0], aBonepawSpawn[i][1], aBonepawSpawn[i][2], aBonepawSpawn[i][3], TEMPSUMMON_DEAD_DESPAWN, 0);

                break;
            }
            case 12:
            {
                SetEscortPaused(true);
                m_bCanDoEvent = true;
                m_uiEventTimer = 1000;
                break;
            }
            case 19:
            {
                SetEscortPaused(true);
                m_bCanDoEvent = true;
                m_uiEventTimer = 5000;
                m_creature->SetFacingTo(3.961f);
                break;
            }
        }

    }

    void UpdateEscortAI(const uint32 uiDiff)
    {
        if (m_bCanDoEvent)
        {
            if (m_uiEventTimer <= uiDiff)
            {
                switch (m_uiEventPhase)
                {
                    case 0:
                    {
                        if (Player* pPlayer = GetPlayerForEscort())
                        {
                            DoScriptText(SAY_MELIZZA_FINISH, m_creature, pPlayer);
                            pPlayer->GroupEventHappens(QUEST_GET_ME_OUT_OF_HERE, m_creature);
                        }

                        m_creature->setFaction(m_uiNormalFaction);
                        SetRun(true);
                        SetEscortPaused(false);
                        m_bCanDoEvent = false;
                        break;
                    }
                    case 1:
                    {
                        DoScriptText(SAY_MELIZZA_1, m_creature);
                        m_uiEventTimer = 4000;
                        break;
                    }
                    case 2:
                    {
                        DoScriptText(SAY_MELIZZA_2, m_creature);
                        m_uiEventTimer = 4000;
                        break;
                    }
                    case 3:
                    {
                        DoScriptText(SAY_MELIZZA_3, m_creature);
                        m_uiEventTimer = 4000;
                        break;
                    }
                    case 4:
                    {
                        SetEscortPaused(false);
                        m_bCanDoEvent = false;
                        break;
                    }
                }

                ++m_uiEventPhase;
            }
            else
                m_uiEventTimer -= uiDiff;

            return;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_melizza_brimbuzzle(Creature* pCreature)
{
    return new npc_melizza_brimbuzzleAI(pCreature);
}

bool QuestAccept_npc_melizza_brimbuzzle(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_GET_ME_OUT_OF_HERE)
        if (npc_melizza_brimbuzzleAI* pEscortAI = dynamic_cast<npc_melizza_brimbuzzleAI*>(pCreature->AI()))
            pEscortAI->Start(false, pPlayer, pQuest);

    return true;
}

void AddSC_desolace()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_aged_dying_ancient_kodo";
    pNewScript->GetAI = &GetAI_npc_aged_dying_ancient_kodo;
    pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_aged_dying_ancient_kodo;
    pNewScript->pGossipHello = &GossipHello_npc_aged_dying_ancient_kodo;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_dalinda_malem";
    pNewScript->GetAI = &GetAI_npc_dalinda_malem;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_dalinda_malem;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_melizza_brimbuzzle";
    pNewScript->GetAI = &GetAI_npc_melizza_brimbuzzle;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_melizza_brimbuzzle;
    pNewScript->RegisterSelf();
}
