/* Orx - Portable Game Engine
 *
 * Copyright (c) 2008-2016 Orx-Project
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

/**
 * @file orxStructure.c
 * @date 08/12/2003
 * @author iarwain@orx-project.org
 *
 */


#include "object/orxStructure.h"

#include "debug/orxDebug.h"
#include "memory/orxBank.h"
#include "debug/orxProfiler.h"


/** Module flags
 */

#define orxSTRUCTURE_KU32_STATIC_FLAG_NONE    0x00000000

#define orxSTRUCTURE_KU32_STATIC_FLAG_READY   0x00000001

#define orxSTRUCTURE_KU32_STATIC_MASK_ALL     0xFFFFFFFF


/** Defines
 */


/***************************************************************************
 * Structure declaration                                                   *
 ***************************************************************************/

/** Internal storage structure
 */
typedef struct __orxSTRUCTURE_STORAGE_t
{
  orxBANK                  *pstStructureBank; /**< Associated structure bank : 4 */
  orxSTRUCTURE_STORAGE_TYPE eType;            /**< Storage type : 4 */

  union
  {
    orxLINKLIST             stLinkList;       /**< Linklist : 20 */
    orxTREE                 stTree;           /**< Tree : 16 */
  };                                          /**< Storage union : 20 */

} orxSTRUCTURE_STORAGE;

/** Internal registration info
 */
typedef struct __orxSTRUCTURE_REGISTER_INFO_t
{
  orxSTRUCTURE_STORAGE_TYPE     eStorageType; /**< Structure storage type : 4 */
  orxU32                        u32Size;      /**< Structure storage size : 8 */
  orxSTRUCTURE_UPDATE_FUNCTION  pfnUpdate;    /**< Structure update callbacks : 12 */
  orxMEMORY_TYPE                eMemoryType;  /**< Structure storage memory type : 16 */

} orxSTRUCTURE_REGISTER_INFO;

/** Static structure
 */
typedef struct __orxSTRUCTURE_STATIC_t
{
  orxSTRUCTURE_STORAGE        astStorage[orxSTRUCTURE_ID_NUMBER];           /**< Structure banks */
  orxSTRUCTURE_REGISTER_INFO  astInfo[orxSTRUCTURE_ID_NUMBER];              /**< Structure info */
  orxU32                      au32InstanceCounter[orxSTRUCTURE_ID_NUMBER];  /**< Structure instance counters */
  orxU32                      u32Flags;                                     /**< Control flags */

} orxSTRUCTURE_STATIC;


/***************************************************************************
 * Static variables                                                        *
 ***************************************************************************/

/** static data
 */
static orxSTRUCTURE_STATIC sstStructure;


/***************************************************************************
 * Private functions                                                       *
 ***************************************************************************/


/***************************************************************************
 * Public functions                                                        *
 ***************************************************************************/

/** Structure module setup
 */
void orxFASTCALL orxStructure_Setup()
{
  /* Adds module dependencies */
  orxModule_AddDependency(orxMODULE_ID_STRUCTURE, orxMODULE_ID_MEMORY);
  orxModule_AddDependency(orxMODULE_ID_STRUCTURE, orxMODULE_ID_BANK);
  orxModule_AddDependency(orxMODULE_ID_STRUCTURE, orxMODULE_ID_PROFILER);
  orxModule_AddDependency(orxMODULE_ID_STRUCTURE, orxMODULE_ID_CONFIG);

  /* Done! */
  return;
}

/** Initializess the structure module
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_Init()
{
  orxU32    i;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(orxSTRUCTURE_ID_NUMBER <= (orxSTRUCTURE_GUID_MASK_STRUCTURE_ID >> orxSTRUCTURE_GUID_SHIFT_STRUCTURE_ID));

  /* Not already Initialized? */
  if(!(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY))
  {
    /* Cleans static controller */
    orxMemory_Zero(&sstStructure, sizeof(orxSTRUCTURE_STATIC));

    /* For all IDs */
    for(i = 0; i < orxSTRUCTURE_ID_NUMBER; i++)
    {
      /* Cleans storage type */
      sstStructure.astStorage[i].eType = orxSTRUCTURE_STORAGE_TYPE_NONE;
    }

    /* Inits Flags */
    sstStructure.u32Flags = orxSTRUCTURE_KU32_STATIC_FLAG_READY;

    /* Updates status */
    eResult = orxSTATUS_SUCCESS;
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Tried to initialize structure module when it was already initialized.");

    /* Already initialized */
    eResult = orxSTATUS_SUCCESS;
  }

  /* Done! */
  return eResult;
}

/** Exits from the structure module
 */
void orxFASTCALL orxStructure_Exit()
{
  /* Initialized? */
  if(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY)
  {
    orxU32 i;

    /* For all structures */
    for(i = 0; i < orxSTRUCTURE_ID_NUMBER; i++)
    {
      /* Depending on storage type */
      switch(sstStructure.astStorage[i].eType)
      {
        case orxSTRUCTURE_STORAGE_TYPE_LINKLIST:
        {
          /* Empties list */
          orxLinkList_Clean(&(sstStructure.astStorage[i].stLinkList));

          break;
        }

        case orxSTRUCTURE_STORAGE_TYPE_TREE:
        {
            /* Empties tree */
            orxTree_Clean(&(sstStructure.astStorage[i].stTree));

            break;
        }

        default:
        {
          break;
        }
      }

      /* Is structure bank valid? */
      if(sstStructure.astStorage[i].pstStructureBank != orxNULL)
      {
        /* Deletes it */
        orxBank_Delete(sstStructure.astStorage[i].pstStructureBank);
      }
    }

    /* Updates flags */
    sstStructure.u32Flags &= ~orxSTRUCTURE_KU32_STATIC_FLAG_READY;
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Tried to exit structure module when it wasn't initialized.");
  }

  /* Done! */
  return;
}

/** Registers a given ID
 * @param[in]   _eStructureID   Concerned structure ID
 * @param[in]   _eStorageType   Storage type to use for this structure type
 * @param[in]   _eMemoryTyp     Memory type to store this structure type
 * @param[in]   _u32Size        Structure size
 * @param[in]   _u32BankSize    Bank (segment) size
 * @param[in]   _pfnUpdate      Structure update function
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_Register(orxSTRUCTURE_ID _eStructureID, orxSTRUCTURE_STORAGE_TYPE _eStorageType, orxMEMORY_TYPE _eMemoryType, orxU32 _u32Size, orxU32 _u32BankSize, const orxSTRUCTURE_UPDATE_FUNCTION _pfnUpdate)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);
  orxASSERT(_u32Size != 0);
  orxASSERT(_u32BankSize != 0);
  orxASSERT(_u32BankSize <= 0xFFFF);
  orxASSERT(_eStorageType < orxSTRUCTURE_STORAGE_TYPE_NUMBER);
  orxASSERT(_eMemoryType < orxMEMORY_TYPE_NUMBER);

  /* Not already registered? */
  if(sstStructure.astInfo[_eStructureID].u32Size == 0)
  {
    /* Creates associated bank */
    sstStructure.astStorage[_eStructureID].pstStructureBank = orxBank_Create((orxU16)_u32BankSize, _u32Size, orxBANK_KU32_FLAG_NONE, _eMemoryType);

    /* Valid? */
    if(sstStructure.astStorage[_eStructureID].pstStructureBank != orxNULL)
    {
      /* Registers it */
      sstStructure.astInfo[_eStructureID].eStorageType  = _eStorageType;
      sstStructure.astInfo[_eStructureID].eMemoryType   = _eMemoryType;
      sstStructure.astInfo[_eStructureID].u32Size       = _u32Size;
      sstStructure.astInfo[_eStructureID].pfnUpdate     = _pfnUpdate;

      sstStructure.astStorage[_eStructureID].eType      = _eStorageType;
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Invalid storage bank created.");
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Structure is already registered.");

    /* Already registered */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Unregisters a given ID
 * @param[in]   _eStructureID   Concerned structure ID
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_Unregister(orxSTRUCTURE_ID _eStructureID)
{
  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);

  /* Registered? */
  if(sstStructure.astInfo[_eStructureID].u32Size != 0)
  {
    /* Deletes structure storage bank */
    orxBank_Delete(sstStructure.astStorage[_eStructureID].pstStructureBank);

    /* Unregisters it */
    orxMemory_Zero(&(sstStructure.astInfo[_eStructureID]), sizeof(orxSTRUCTURE_REGISTER_INFO));
    sstStructure.astStorage[_eStructureID].pstStructureBank = orxNULL;
    sstStructure.astStorage[_eStructureID].eType            = orxSTRUCTURE_STORAGE_TYPE_NONE;
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Structure is already registered.");
  }

  /* Done! */
  return orxSTATUS_SUCCESS;
}

/** Creates a clean structure for given type
 * @param[in]   _eStructureID   Concerned structure ID
 * @return      orxSTRUCTURE / orxNULL
 */
orxSTRUCTURE *orxFASTCALL orxStructure_Create(orxSTRUCTURE_ID _eStructureID)
{
  orxSTRUCTURE *pstStructure = orxNULL;

  /* Profiles */
  orxPROFILER_PUSH_MARKER("orxStructure_Create");

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);

  /* Is structure type registered? */
  if(sstStructure.astInfo[_eStructureID].u32Size != 0)
  {
    orxSTRUCTURE *pstPrevious;
    orxU32        u32ItemID;

    /* Creates structure */
    pstStructure = (orxSTRUCTURE *)orxBank_AllocateIndexed(sstStructure.astStorage[_eStructureID].pstStructureBank, &u32ItemID, (void **)&pstPrevious);

    /* Valid? */
    if(pstStructure != orxNULL)
    {
      orxSTATUS eResult;

      /* Cleans whole structure */
      orxMemory_Zero(pstStructure, sstStructure.astInfo[_eStructureID].u32Size);

      /* Depending on storage type */
      switch(sstStructure.astStorage[_eStructureID].eType)
      {
        case orxSTRUCTURE_STORAGE_TYPE_LINKLIST:
        {
          /* Has a previous element? */
          if(pstPrevious != orxNULL)
          {
            /* Adds it to list */
            eResult = orxLinkList_AddAfter(&(pstPrevious->stStorage.stLinkListNode), &(pstStructure->stStorage.stLinkListNode));
          }
          else
          {
            /* Adds it to list */
            eResult = orxLinkList_AddStart(&(sstStructure.astStorage[_eStructureID].stLinkList), &(pstStructure->stStorage.stLinkListNode));
          }

          break;
        }

        case orxSTRUCTURE_STORAGE_TYPE_TREE:
        {
          /* No root yet? */
          if(orxTree_GetRoot(&(sstStructure.astStorage[_eStructureID].stTree)) == orxNULL)
          {
            /* Adds root to tree */
            eResult = orxTree_AddRoot(&(sstStructure.astStorage[_eStructureID].stTree), &(pstStructure->stStorage.stTreeNode));
          }
          else
          {
            /* Adds node to tree */
            eResult = orxTree_AddChild(orxTree_GetRoot(&(sstStructure.astStorage[_eStructureID].stTree)), &(pstStructure->stStorage.stTreeNode));
          }

          break;
        }

        default:
        {
          /* Logs message */
          orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Invalid structure storage type.");

          /* Wrong type */
          eResult = orxSTATUS_FAILURE;
        }
      }

      /* Successful? */
      if(eResult != orxSTATUS_FAILURE)
      {
        /* Checks */
        orxASSERT(_eStructureID <= (orxU32)(orxSTRUCTURE_GUID_MASK_STRUCTURE_ID >> orxSTRUCTURE_GUID_SHIFT_STRUCTURE_ID));
        orxASSERT(u32ItemID <= (orxU32)(orxSTRUCTURE_GUID_MASK_ITEM_ID >> orxSTRUCTURE_GUID_SHIFT_ITEM_ID));
        orxASSERT(sstStructure.au32InstanceCounter[_eStructureID] <= (orxU32)(orxSTRUCTURE_GUID_MASK_INSTANCE_ID >> orxSTRUCTURE_GUID_SHIFT_INSTANCE_ID));

        /* Stores GUID */
        pstStructure->u64GUID = ((orxU64)_eStructureID << orxSTRUCTURE_GUID_SHIFT_STRUCTURE_ID)
                              | ((orxU64)u32ItemID << orxSTRUCTURE_GUID_SHIFT_ITEM_ID)
                              | ((orxU64)sstStructure.au32InstanceCounter[_eStructureID] << orxSTRUCTURE_GUID_SHIFT_INSTANCE_ID);

        /* Cleans owner GUID */
        pstStructure->u64OwnerGUID = orxU64_UNDEFINED;

        /* Updates instance ID */
        sstStructure.au32InstanceCounter[_eStructureID] = (sstStructure.au32InstanceCounter[_eStructureID] + 1) & (orxSTRUCTURE_GUID_MASK_INSTANCE_ID >> orxSTRUCTURE_GUID_SHIFT_INSTANCE_ID);
      }
      else
      {
        /* Logs message */
        orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Failed to add node to list.");

        /* Frees allocated structure */
        orxBank_Free(sstStructure.astStorage[_eStructureID].pstStructureBank, pstStructure);

        /* Not created */
        pstStructure = orxNULL;
      }
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Failed to allocate from structure bank.");
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Structure type is not registered.");
  }

  /* Profiles */
  orxPROFILER_POP_MARKER();

  /* Done! */
  return pstStructure;
}

/** Deletes a structure (needs to be cleaned beforehand)
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_Delete(void *_pStructure)
{
  orxSTRUCTURE_ID eStructureID;
  orxSTRUCTURE   *pstStructure;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxASSERT((((orxSTRUCTURE *)_pStructure)->u64OwnerGUID == orxU64_UNDEFINED) || (((orxSTRUCTURE *)_pStructure)->u64OwnerGUID == ((orxSTRUCTURE *)_pStructure)->u64GUID));

  /* Gets structure */
  pstStructure = orxSTRUCTURE(_pStructure);

  /* Depending on type */
  switch(sstStructure.astStorage[orxStructure_GetID(pstStructure)].eType)
  {
    case orxSTRUCTURE_STORAGE_TYPE_LINKLIST:
    {
      /* Removes node from list */
      orxLinkList_Remove(&(pstStructure->stStorage.stLinkListNode));

      break;
    }

    case orxSTRUCTURE_STORAGE_TYPE_TREE:
    {
      /* Removes node from list */
      orxTree_Remove(&(pstStructure->stStorage.stTreeNode));

      break;
    }

    default:
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Invalid structure storage type.");

      break;
    }
  }

  /* Gets structure ID */
  eStructureID = orxStructure_GetID(_pStructure);

  /* Tags structure as deleted */
  orxSTRUCTURE(_pStructure)->u64GUID = orxSTRUCTURE_GUID_MAGIC_TAG_DELETED;

  /* Deletes structure */
  orxBank_Free(sstStructure.astStorage[eStructureID].pstStructureBank, _pStructure);

  /* Done! */
  return orxSTATUS_SUCCESS;
}

/** Gets structure storage type
 * @param[in]   _eStructureID   Concerned structure ID
 * @return      orxSTRUCTURE_STORAGE_TYPE
 */
orxSTRUCTURE_STORAGE_TYPE orxFASTCALL orxStructure_GetStorageType(orxSTRUCTURE_ID _eStructureID)
{
  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);

  /* Returns it */
  return(sstStructure.astStorage[_eStructureID].eType);
}

/** Gets given type structure counter
 * @param[in]   _eStructureID   Concerned structure ID
 * @return      orxU32 / orxU32_UNDEFINED
 */
orxU32 orxFASTCALL orxStructure_GetCounter(orxSTRUCTURE_ID _eStructureID)
{
  orxU32 u32Result = orxU32_UNDEFINED;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);

  /* Depending on type */
  switch(sstStructure.astStorage[_eStructureID].eType)
  {
    case orxSTRUCTURE_STORAGE_TYPE_LINKLIST:
    {
      /* Gets counter */
      u32Result = orxLinkList_GetCounter(&(sstStructure.astStorage[_eStructureID].stLinkList));

      break;
    }

    case orxSTRUCTURE_STORAGE_TYPE_TREE:
    {
      /* Gets counter */
      u32Result = orxTree_GetCounter(&(sstStructure.astStorage[_eStructureID].stTree));

      break;
    }

    default:
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Invalid structure storage type.");

      break;
    }
  }

  /* Done ! */
  return u32Result;
}

/** Updates structure if update function was registered for the structure type
 * @param[in]   _pStructure    Concerned structure
 * @param[in]   _pCaller       Caller structure
 * @param[in]   _pstClockInfo   Update associated clock info
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_Update(void *_pStructure, const void *_pCaller, const orxCLOCK_INFO *_pstClockInfo)
{
  orxU32    u32ID;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);

  /* Gets structure ID */
  u32ID = orxStructure_GetID(_pStructure);

  /* Is structure registered? */
  if(sstStructure.astInfo[u32ID].u32Size != 0)
  {
    /* Is an update function registered? */
    if(sstStructure.astInfo[u32ID].pfnUpdate != orxNULL)
    {
      /* Calls it */
      eResult = sstStructure.astInfo[u32ID].pfnUpdate(((orxSTRUCTURE *)_pStructure), ((orxSTRUCTURE *)_pCaller), _pstClockInfo);
    }
    else
    {
      /* Updates result */
      eResult = orxSTATUS_SUCCESS;
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Structure is not registered.");
  }

  /* Done! */
  return eResult;
}

/** Gets structure given its GUID
 * @param[in]   _u64GUID        Structure's GUID
 * @return      orxSTRUCTURE / orxNULL if not found/alive
 */
orxSTRUCTURE *orxFASTCALL orxStructure_Get(orxU64 _u64GUID)
{
  orxU64        u64StructureID;
  orxSTRUCTURE *pstResult;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);

  /* Gets structure ID */
  u64StructureID = (_u64GUID & orxSTRUCTURE_GUID_MASK_STRUCTURE_ID) >> orxSTRUCTURE_GUID_SHIFT_STRUCTURE_ID;

  /* Valid? */
  if(u64StructureID < orxSTRUCTURE_ID_NUMBER)
  {
    /* Gets structure at index */
    pstResult = (orxSTRUCTURE *)orxBank_GetAtIndex(sstStructure.astStorage[u64StructureID].pstStructureBank, (orxU32)((_u64GUID & orxSTRUCTURE_GUID_MASK_ITEM_ID) >> orxSTRUCTURE_GUID_SHIFT_ITEM_ID));

    /* Valid? */
    if(pstResult != orxNULL)
    {
      /* Invalid instance ID? */
      if((pstResult->u64GUID & orxSTRUCTURE_GUID_MASK_INSTANCE_ID) != (_u64GUID & orxSTRUCTURE_GUID_MASK_INSTANCE_ID))
      {
        /* Clears result */
        pstResult = orxNULL;
      }
    }
  }
  else
  {
    /* Clears result */
    pstResult = orxNULL;
  }

  /* Done! */
  return pstResult;
}

/** Gets structure's owner
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTRUCTURE / orxNULL if not found/alive
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetOwner(const void *_pStructure)
{
  orxSTRUCTURE *pstResult;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);

  /* Updates result */
  pstResult = orxStructure_Get(((orxSTRUCTURE *)_pStructure)->u64OwnerGUID);

  /* Done! */
  return pstResult;
}

/** Sets structure owner
 * @param[in]   _pStructure    Concerned structure
 * @param[in]   _pParent       Structure to set as owner
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_SetOwner(void *_pStructure, void *_pOwner)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);

  /* Has owner? */
  if(_pOwner != orxNULL)
  {
    /* Updates structure's owner GUID */
    orxSTRUCTURE(_pStructure)->u64OwnerGUID = orxStructure_GetGUID(_pOwner);
  }
  else
  {
    /* Removes owner */
    orxSTRUCTURE(_pStructure)->u64OwnerGUID = orxU64_UNDEFINED;
  }

  /* Done! */
  return eResult;
}

/** Gets first stored structure (first list cell or tree root depending on storage type)
 * @param[in]   _eStructureID   Concerned structure ID
 * @return      orxSTRUCTURE
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetFirst(orxSTRUCTURE_ID _eStructureID)
{
  orxSTRUCTURE *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);

  /* Depending on type */
  switch(sstStructure.astStorage[_eStructureID].eType)
  {
    case orxSTRUCTURE_STORAGE_TYPE_LINKLIST:
    {
      /* Gets node from list */
      pstResult = (orxSTRUCTURE *)orxLinkList_GetFirst(&(sstStructure.astStorage[_eStructureID].stLinkList));

      /* Valid? */
      if(pstResult != orxNULL)
      {
        /* Updates result */
        pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stLinkListNode, pstResult);
      }

      break;
    }

    case orxSTRUCTURE_STORAGE_TYPE_TREE:
    {
      /* Gets node from tree */
      pstResult = (orxSTRUCTURE *)orxTree_GetRoot(&(sstStructure.astStorage[_eStructureID].stTree));

      /* Valid? */
      if(pstResult != orxNULL)
      {
        /* Updates result */
        pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stTreeNode, pstResult);
      }

      break;
    }

    default:
    {
      /* No node found */
      pstResult = orxNULL;

      break;
    }
  }

  /* Done! */
  return pstResult;
}

/** Gets last stored structure (last list cell or tree root depending on storage type)
 * @param[in] _eStructureID      Concerned structure ID
 * return orxSTRUCTURE / orxNULL
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetLast(orxSTRUCTURE_ID _eStructureID)
{
  orxSTRUCTURE *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_eStructureID < orxSTRUCTURE_ID_NUMBER);

  /* Depending on type */
  switch(sstStructure.astStorage[_eStructureID].eType)
  {
    case orxSTRUCTURE_STORAGE_TYPE_LINKLIST:
    {
      /* Gets node from list */
      pstResult = (orxSTRUCTURE *)orxLinkList_GetLast(&(sstStructure.astStorage[_eStructureID].stLinkList));

      /* Valid? */
      if(pstResult != orxNULL)
      {
        /* Updates result */
        pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stLinkListNode, pstResult);
      }

      break;
    }

    case orxSTRUCTURE_STORAGE_TYPE_TREE:
    {
      /* Gets node from tree */
      pstResult = (orxSTRUCTURE *)orxTree_GetRoot(&(sstStructure.astStorage[_eStructureID].stTree));

      /* Valid? */
      if(pstResult != orxNULL)
      {
        /* Updates result */
        pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stTreeNode, pstResult);
      }

      break;
    }

    default:
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Invalid structure storage type.");

      /* No node found */
      pstResult = orxNULL;

      break;
    }
  }

  /* Done! */
  return pstResult;
}

/** Gets structure tree parent
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTRUCTURE
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetParent(const void *_pStructure)
{
  orxSTRUCTURE *pstStructure, *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxASSERT(sstStructure.astStorage[orxStructure_GetID(_pStructure)].eType == orxSTRUCTURE_STORAGE_TYPE_TREE);

  /* Gets structure */
  pstStructure = orxSTRUCTURE(_pStructure);

  /* Gets parent */
  pstResult = (orxSTRUCTURE *)orxTree_GetParent(&(pstStructure->stStorage.stTreeNode));

  /* Valid? */
  if(pstResult != orxNULL)
  {
    /* Updates result */
    pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stTreeNode, pstResult);
  }

  /* Done! */
  return pstResult;
}

/** Gets structure tree child
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTRUCTURE
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetChild(const void *_pStructure)
{
  orxSTRUCTURE *pstStructure, *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxASSERT(sstStructure.astStorage[orxStructure_GetID(_pStructure)].eType == orxSTRUCTURE_STORAGE_TYPE_TREE);

  /* Gets structure */
  pstStructure = orxSTRUCTURE(_pStructure);

  /* Gets child */
  pstResult = (orxSTRUCTURE *)orxTree_GetChild(&(pstStructure->stStorage.stTreeNode));

  /* Valid? */
  if(pstResult != orxNULL)
  {
    /* Updates result */
    pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stTreeNode, pstResult);
  }

  /* Done! */
  return pstResult;
}

/** Gets structure tree sibling
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTRUCTURE
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetSibling(const void *_pStructure)
{
  orxSTRUCTURE *pstStructure, *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxASSERT(sstStructure.astStorage[orxStructure_GetID(_pStructure)].eType == orxSTRUCTURE_STORAGE_TYPE_TREE);

  /* Gets structure */
  pstStructure = orxSTRUCTURE(_pStructure);

  /* Gets sibling */
  pstResult = (orxSTRUCTURE *)orxTree_GetSibling(&(pstStructure->stStorage.stTreeNode));

  /* Valid? */
  if(pstResult != orxNULL)
  {
    /* Updates result */
    pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stTreeNode, pstResult);
  }

  /* Done! */
  return pstResult;
}

/** Gets structure list previous
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTRUCTURE
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetPrevious(const void *_pStructure)
{
  orxSTRUCTURE *pstStructure, *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxASSERT(sstStructure.astStorage[orxStructure_GetID(_pStructure)].eType == orxSTRUCTURE_STORAGE_TYPE_LINKLIST);

  /* Gets structure */
  pstStructure = orxSTRUCTURE(_pStructure);

  /* Gets previous */
  pstResult = (orxSTRUCTURE *)orxLinkList_GetPrevious(&(pstStructure->stStorage.stLinkListNode));

  /* Valid? */
  if(pstResult != orxNULL)
  {
    /* Updates result */
    pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stLinkListNode, pstResult);
  }

  /* Done! */
  return pstResult;
}

/** Gets structure list next
 * @param[in]   _pStructure    Concerned structure
 * @return      orxSTRUCTURE
 */
orxSTRUCTURE *orxFASTCALL orxStructure_GetNext(const void *_pStructure)
{
  orxSTRUCTURE *pstStructure, *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxASSERT(sstStructure.astStorage[orxStructure_GetID(_pStructure)].eType == orxSTRUCTURE_STORAGE_TYPE_LINKLIST);

  /* Gets structure */
  pstStructure = orxSTRUCTURE(_pStructure);

  /* Gets next */
  pstResult = (orxSTRUCTURE *)orxLinkList_GetNext(&(pstStructure->stStorage.stLinkListNode));

  /* Valid? */
  if(pstResult != orxNULL)
  {
    /* Updates result */
    pstResult = orxSTRUCT_GET_FROM_FIELD(orxSTRUCTURE, stStorage.stLinkListNode, pstResult);
  }

  /* Done! */
  return pstResult;
}

/** Sets structure tree parent
 * @param[in]   _pStructure    Concerned structure
 * @param[in]   _pParent       Structure to set as parent
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxStructure_SetParent(void *_pStructure, void *_pParent)
{
  orxSTRUCTURE *pstStructure, *pstParent;
  orxSTATUS     eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstStructure.u32Flags & orxSTRUCTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pStructure);
  orxSTRUCTURE_ASSERT(_pParent);
  orxASSERT(sstStructure.astStorage[orxStructure_GetID(_pParent)].eType == orxSTRUCTURE_STORAGE_TYPE_TREE);

  /* Gets structures */
  pstStructure = orxSTRUCTURE(_pStructure);
  pstParent    = orxSTRUCTURE(_pParent);

  /* Is storage type correct? */
  if(sstStructure.astStorage[orxStructure_GetID(pstStructure)].eType == orxSTRUCTURE_STORAGE_TYPE_TREE)
  {
    /* Moves it */
    eResult = orxTree_MoveAsChild(&(pstParent->stStorage.stTreeNode), &(pstStructure->stStorage.stTreeNode));
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_OBJECT, "Structure [%s] is not stored as a list.", orxStructure_GetIDString(orxStructure_GetID(pstStructure)));

    /* Not done */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}
