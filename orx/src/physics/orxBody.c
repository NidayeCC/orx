/* Orx - Portable Game Engine
 *
 * Orx is the legal property of its developers, whose names
 * are listed in the COPYRIGHT file distributed 
 * with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file orxBody.c
 * @date 10/03/2008
 * @author iarwain@orx-project.org
 *
 * @todo
 */


#include "physics/orxBody.h"
#include "physics/orxPhysics.h"
#include "core/orxConfig.h"
#include "object/orxObject.h"
#include "object/orxFrame.h"
#include "utils/orxString.h"

#include "debug/orxDebug.h"
#include "memory/orxMemory.h"


/** Body flags
 */
#define orxBODY_KU32_FLAG_NONE                0x00000000  /**< No flags */

#define orxBODY_KU32_FLAG_HAS_DATA            0x00000001  /**< Has data flag */
#define orxBODY_KU32_FLAG_USE_TEMPLATE        0x00000002  /**< Use body template flag */
#define orxBODY_KU32_FLAG_USE_PART_TEMPLATE   0x00000004  /**< Use body part template flag */

#define orxBODY_KU32_MASK_ALL                 0xFFFFFFFF  /**< User all ID mask */


/** Module flags
 */
#define orxBODY_KU32_STATIC_FLAG_NONE         0x00000000  /**< No flags */

#define orxBODY_KU32_STATIC_FLAG_READY        0x10000000  /**< Ready flag */

#define orxBODY_KU32_MASK_ALL                 0xFFFFFFFF  /**< All mask */


/** Misc defines
 */
#define orxBODY_KZ_CONFIG_POSITION            "Position"
#define orxBODY_KZ_CONFIG_ROTATION            "Rotation"
#define orxBODY_KZ_CONFIG_INERTIA             "Inertia"
#define orxBODY_KZ_CONFIG_MASS                "Mass"
#define orxBODY_KZ_CONFIG_LINEAR_DAMPING      "LinearDamping"
#define orxBODY_KZ_CONFIG_ANGULAR_DAMPING     "AngularDamping"
#define orxBODY_KZ_CONFIG_FIXED_ROTATION      "FixedRotation"
#define orxBODY_KZ_CONFIG_HIGH_SPEED          "HighSpeed"
#define orxBODY_KZ_CONFIG_DYNAMIC             "Dynamic"
#define orxBODY_KZ_CONFIG_PART                "Part"
#define orxBODY_KZ_CONFIG_FRICTION            "Friction"
#define orxBODY_KZ_CONFIG_RESTITUTION         "Restitution"
#define orxBODY_KZ_CONFIG_DENSITY             "Density"
#define orxBODY_KZ_CONFIG_SELF_FLAGS          "SelfFlags"
#define orxBODY_KZ_CONFIG_CHECK_MASK          "CheckMask"
#define orxBODY_KZ_CONFIG_TYPE                "Type"
#define orxBODY_KZ_CONFIG_SOLID               "Solid"
#define orxBODY_KZ_CONFIG_TOP_LEFT            "TopLeft"
#define orxBODY_KZ_CONFIG_BOTTOM_RIGHT        "BottomRight"
#define orxBODY_KZ_CONFIG_CENTER              "Center"
#define orxBODY_KZ_CONFIG_RADIUS              "Radius"

#define orxBODY_KZ_FULL                       "full"
#define orxBODY_KZ_TYPE_SPHERE                "sphere"
#define orxBODY_KZ_TYPE_BOX                   "box"


/***************************************************************************
 * Structure declaration                                                   *
 ***************************************************************************/

/** Body part structure
 */
struct __orxBODY_PART_t
{
  orxPHYSICS_BODY_PART *pstData;                                /**< Data structure : 4 */
  orxBODY_PART_DEF      stDef;                                  /**< Definition : 60 */

  orxPAD(60)
};

/** Body structure
 */
struct __orxBODY_t
{
  orxSTRUCTURE            stStructure;                                /**< Public structure, first structure member : 16 */
  orxPHYSICS_BODY        *pstData;                                    /**< Physics body data : 20 */
  orxCONST orxSTRUCTURE  *pstOwner;                                   /**< Owner structure : 24 */
  orxBODY_PART            astPartList[orxBODY_KU32_PART_MAX_NUMBER];  /**< Body part structure list : 264 */

  orxPAD(264)
};

/** Static structure
 */
typedef struct __orxBODY_STATIC_t
{
  orxU32            u32Flags;                                         /**< Control flags */
  orxBODY_DEF       stBodyTemplate;                                   /**< Body template */
  orxBODY_PART_DEF  stBodyPartTemplate;                               /**< Body part template */

} orxBODY_STATIC;


/***************************************************************************
 * Static variables                                                        *
 ***************************************************************************/

/** Static data
 */
orxSTATIC orxBODY_STATIC sstBody;


/***************************************************************************
 * Private functions                                                       *
 ***************************************************************************/

/** Deletes all bodies
 */
orxSTATIC orxINLINE orxVOID orxBody_DeleteAll()
{
  orxREGISTER orxBODY *pstBody;

  /* Gets first body */
  pstBody = orxBODY(orxStructure_GetFirst(orxSTRUCTURE_ID_BODY));

  /* Non empty? */
  while(pstBody != orxNULL)
  {
    /* Deletes Body */
    orxBody_Delete(pstBody);

    /* Gets first Body */
    pstBody = orxBODY(orxStructure_GetFirst(orxSTRUCTURE_ID_BODY));
  }

  return;
}

/** Updates the Body (Callback for generic structure update calling)
 * @param[in]   _pstStructure                 Generic Structure or the concerned Body
 * @param[in]   _pstCaller                    Structure of the caller
 * @param[in]   _pstClockInfo                 Clock info used for time updates
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATIC orxSTATUS orxFASTCALL orxBody_Update(orxSTRUCTURE *_pstStructure, orxCONST orxSTRUCTURE *_pstCaller, orxCONST orxCLOCK_INFO *_pstClockInfo)
{
  orxBODY    *pstBody;
  orxOBJECT  *pstObject;
  orxSTATUS   eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstStructure);

  /* Gets body */
  pstBody = orxBODY(_pstStructure);

  /* Gets calling object */
  pstObject = orxOBJECT(_pstCaller);

  /* Has data? */
  if(orxStructure_TestFlags(pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    orxFRAME *pstFrame;

    /* Gets its frame */
    pstFrame = orxOBJECT_GET_STRUCTURE(pstObject, FRAME);

    /* Is a root's children frame? */
    if(orxFrame_IsRootChild(pstFrame) != orxFALSE)
    {
      orxVECTOR vPosition;
      orxFLOAT  fZBackup, fRotation;

      /* Gets current position */
      orxFrame_GetPosition(pstFrame, orxFRAME_SPACE_LOCAL, &vPosition);

      /* Backups its Z */
      fZBackup = vPosition.fZ;

      /* Gets body up-to-date position & rotation */
      orxPhysics_GetPosition(pstBody->pstData, &vPosition);
      fRotation = orxPhysics_GetRotation(pstBody->pstData);

      /* Restores Z */
      vPosition.fZ = fZBackup;

      /* Updates position & rotation */
      orxFrame_SetPosition(pstFrame, &vPosition);
      orxFrame_SetRotation(pstFrame, fRotation);
    }
  }

  /* Done! */
  return eResult;
}


/***************************************************************************
 * Public functions                                                        *
 ***************************************************************************/

/** Body module setup
 */
orxVOID orxBody_Setup()
{
  /* Adds module dependencies */
  orxModule_AddDependency(orxMODULE_ID_BODY, orxMODULE_ID_MEMORY);
  orxModule_AddDependency(orxMODULE_ID_BODY, orxMODULE_ID_STRUCTURE);
  orxModule_AddDependency(orxMODULE_ID_BODY, orxMODULE_ID_PHYSICS);
  orxModule_AddDependency(orxMODULE_ID_BODY, orxMODULE_ID_FRAME);
  orxModule_AddDependency(orxMODULE_ID_BODY, orxMODULE_ID_CONFIG);

  return;
}

/** Inits the Body module
 */
orxSTATUS orxBody_Init()
{
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Not already Initialized? */
  if((sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY) == orxBODY_KU32_STATIC_FLAG_NONE)
  {
    /* Cleans static controller */
    orxMemory_Zero(&sstBody, sizeof(orxBODY_STATIC));

    /* Registers structure type */
    eResult = orxSTRUCTURE_REGISTER(BODY, orxSTRUCTURE_STORAGE_TYPE_LINKLIST, orxMEMORY_TYPE_MAIN, &orxBody_Update);
  }
  else
  {
    /* !!! MSG !!! */

    /* Already initialized */
    eResult = orxSTATUS_SUCCESS;
  }

  /* Initialized? */
  if(eResult == orxSTATUS_SUCCESS)
  {
    /* Inits Flags */
    sstBody.u32Flags = orxBODY_KU32_STATIC_FLAG_READY;
  }
  else
  {
    /* !!! MSG !!! */
  }

  /* Done! */
  return eResult;
}

/** Exits from the Body module
 */
orxVOID orxBody_Exit()
{
  /* Initialized? */
  if(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY)
  {
    /* Deletes body list */
    orxBody_DeleteAll();

    /* Unregisters structure type */
    orxStructure_Unregister(orxSTRUCTURE_ID_BODY);

    /* Updates flags */
    sstBody.u32Flags &= ~orxBODY_KU32_STATIC_FLAG_READY;
  }
  else
  {
    /* !!! MSG !!! */
  }

  return;
}

/** Creates an empty body
 * @param[in]   _pstOwner                     Body's owner used for collision callbacks (usually an orxOBJECT)
 * @param[in]   _pstBodyDef                   Body definition
 * @return      Created orxBODY / orxNULL
 */
orxBODY *orxFASTCALL orxBody_Create(orxCONST orxSTRUCTURE *_pstOwner, orxCONST orxBODY_DEF *_pstBodyDef)
{
  orxBODY *pstBody;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxASSERT(orxOBJECT(_pstOwner));
  orxASSERT((_pstBodyDef != orxNULL) || (orxFLAG_TEST(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_TEMPLATE)));

  /* Creates body */
  pstBody = orxBODY(orxStructure_Create(orxSTRUCTURE_ID_BODY));

  /* Valid? */
  if(pstBody != orxNULL)
  {
    orxBODY_DEF           stMergedDef;
    orxCONST orxBODY_DEF *pstSelectedDef;

    /* Inits flags */
    orxStructure_SetFlags(pstBody, orxBODY_KU32_FLAG_NONE, orxBODY_KU32_MASK_ALL);

    /* Uses template? */
    if(orxFLAG_TEST(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_TEMPLATE))
    {
      /* Has specific definition? */
      if(_pstBodyDef != orxNULL)
      {
        /* Cleans merged def */
        orxMemory_Zero(&stMergedDef, sizeof(orxBODY_DEF));

        /* Merges template with specialized definition */
        orxVector_Copy(&(stMergedDef.vPosition), (orxVector_IsNull(&(_pstBodyDef->vPosition)) == orxFALSE) ? &(_pstBodyDef->vPosition) : &(sstBody.stBodyTemplate.vPosition));
        stMergedDef.fRotation       = (_pstBodyDef->fRotation != 0.0f) ? _pstBodyDef->fRotation : sstBody.stBodyTemplate.fRotation;
        stMergedDef.fInertia        = (_pstBodyDef->fInertia > 0.0f) ? _pstBodyDef->fInertia : sstBody.stBodyTemplate.fInertia;
        stMergedDef.fMass           = (_pstBodyDef->fMass > 0.0f) ? _pstBodyDef->fMass : sstBody.stBodyTemplate.fMass;
        stMergedDef.fLinearDamping  = (_pstBodyDef->fLinearDamping > 0.0f) ? _pstBodyDef->fLinearDamping : sstBody.stBodyTemplate.fLinearDamping;
        stMergedDef.fAngularDamping = (_pstBodyDef->fAngularDamping > 0.0f) ? _pstBodyDef->fAngularDamping : sstBody.stBodyTemplate.fAngularDamping;
        stMergedDef.u32Flags        = (_pstBodyDef->u32Flags != orxBODY_DEF_KU32_FLAG_NONE) ? _pstBodyDef->u32Flags : sstBody.stBodyTemplate.u32Flags;

        /* Selects it */
        pstSelectedDef = &stMergedDef;
      }
      else
      {
        /* Selects template */
        pstSelectedDef = &(sstBody.stBodyTemplate);
      }
    }
    else
    {
      /* Selects specialized definition */
      pstSelectedDef = _pstBodyDef;
    }

    /* Creates physics body */
    pstBody->pstData = orxPhysics_CreateBody(pstBody, pstSelectedDef);

    /* Valid? */
    if(pstBody->pstData != orxNULL)
    {
      /* Stores owner */
      pstBody->pstOwner = _pstOwner;

      /* Updates flags */
      orxStructure_SetFlags(pstBody, orxBODY_KU32_FLAG_HAS_DATA, orxBODY_KU32_FLAG_NONE);
    }
    else
    {
      /* !!! MSG !!! */

      /* Deletes allocated structure */
      orxStructure_Delete(pstBody);
      pstBody = orxNULL;
    }
  }

  /* Done! */
  return pstBody;
}

/** Creates a body from config
 * @param[in]   _pstOwner                     Body's owner used for collision callbacks (usually an orxOBJECT)
 * @param[in]   _zConfigID                    Body config ID
 * @return      Created orxGRAPHIC / orxNULL
 */
orxBODY *orxFASTCALL orxBody_CreateFromConfig(orxCONST orxSTRUCTURE *_pstOwner, orxCONST orxSTRING _zConfigID)
{
  orxSTRING zPreviousSection;
  orxBODY  *pstResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstOwner);
  orxASSERT((_zConfigID != orxNULL) && (*_zConfigID != *orxSTRING_EMPTY));

  /* Gets previous config section */
  zPreviousSection = orxConfig_GetCurrentSection();

  /* Selects section */
  if(orxConfig_SelectSection(_zConfigID) != orxSTATUS_FAILURE)
  {
    orxBODY_DEF stBodyDef;

    /* Clears body definition */
    orxMemory_Zero(&stBodyDef, sizeof(orxBODY_DEF));

    /* Inits it */
    orxConfig_GetVector(orxBODY_KZ_CONFIG_POSITION, &(stBodyDef.vPosition));
    stBodyDef.fRotation       = orxConfig_GetFloat(orxBODY_KZ_CONFIG_ROTATION);
    stBodyDef.fInertia        = orxConfig_GetFloat(orxBODY_KZ_CONFIG_INERTIA);
    stBodyDef.fMass           = orxConfig_GetFloat(orxBODY_KZ_CONFIG_MASS);
    stBodyDef.fLinearDamping  = orxConfig_GetFloat(orxBODY_KZ_CONFIG_LINEAR_DAMPING);
    stBodyDef.fAngularDamping = orxConfig_GetFloat(orxBODY_KZ_CONFIG_ANGULAR_DAMPING);
    stBodyDef.u32Flags        = orxBODY_DEF_KU32_FLAG_2D;
    if(orxConfig_GetBool(orxBODY_KZ_CONFIG_FIXED_ROTATION) != orxFALSE)
    {
      stBodyDef.u32Flags |= orxBODY_DEF_KU32_FLAG_FIXED_ROTATION;
    }
    if(orxConfig_GetBool(orxBODY_KZ_CONFIG_HIGH_SPEED) != orxFALSE)
    {
      stBodyDef.u32Flags |= orxBODY_DEF_KU32_FLAG_HIGH_SPEED;
    }
    if(orxConfig_GetBool(orxBODY_KZ_CONFIG_DYNAMIC) != orxFALSE)
    {
      stBodyDef.u32Flags |= orxBODY_DEF_KU32_FLAG_DYNAMIC;
    }

    /* Creates body */
    pstResult = orxBody_Create(_pstOwner, &stBodyDef);

    /* Valid? */
    if(pstResult != orxNULL)
    {
      orxCHAR acPartID[16];
      orxU32  i;

      /* Clears buffer */
      orxMemory_Zero(acPartID, 16 * sizeof(orxCHAR));

      /* For all parts */
      for(i = 1; i <= orxBODY_KU32_PART_MAX_NUMBER; i++)
      {
        orxSTRING zPartName;

        /* Gets part name */
        orxString_Print(acPartID, "%s%d", orxBODY_KZ_CONFIG_PART, i);

        /* Has part? */
        if(orxConfig_HasValue(acPartID) != orxFALSE)
        {
          /* Gets part name */
          zPartName = orxConfig_GetString(acPartID);

          /* Adds part */
          orxBody_AddPartFromConfig(pstResult, i - 1, zPartName);
        }
        else
        {
          break;
        }
      }
    }

    /* Restores previous section */
    orxConfig_SelectSection(zPreviousSection);
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    pstResult = orxNULL;
  }

  /* Done! */
  return pstResult;
}

/** Deletes a body
 * @param[in]   _pstBody     Body to delete
 */
orxSTATUS orxFASTCALL orxBody_Delete(orxBODY *_pstBody)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Not referenced? */
  if(orxStructure_GetRefCounter(_pstBody) == 0)
  {
    orxU32 i;

    /* For all data structure */
    for(i = 0; i < orxBODY_KU32_PART_MAX_NUMBER; i++)
    {
      /* Cleans it */
      orxBody_RemovePart(_pstBody, i);
    }

    /* Has data? */
    if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
    {
      /* Deletes physics body */
      orxPhysics_DeleteBody(_pstBody->pstData);
    }

    /* Deletes structure */
    orxStructure_Delete(_pstBody);
  }
  else
  {
    /* !!! MSG !!! */

    /* Referenced by others */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Gets a body owner
 * @param[in]   _pstBody        Concerned body
 * @return      orxSTRUCTURE / orxNULL
 */
orxSTRUCTURE *orxFASTCALL orxBody_GetOwner(orxCONST orxBODY *_pstBody)
{
  orxSTRUCTURE *pstResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Updates result */
  pstResult = orxSTRUCTURE(_pstBody->pstOwner);

  /* Done! */
  return pstResult;
}

/** Adds a body part
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _u32Index       Part index (should be less than orxBODY_KU32_PART_MAX_NUMBER)
 * @param[in]   _pstPartDef     Body part definition
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_AddPart(orxBODY *_pstBody, orxU32 _u32Index, orxCONST orxBODY_PART_DEF *_pstBodyPartDef)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT((_pstBodyPartDef != orxNULL) || (orxFLAG_TEST(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_PART_TEMPLATE)));
  orxASSERT(_u32Index < orxBODY_KU32_PART_MAX_NUMBER);

  /* Had previous part? */
  if(_pstBody->astPartList[_u32Index].pstData != orxNULL)
  {
    /* Removes it */
    eResult = orxBody_RemovePart(_pstBody, _u32Index);
  }

  /* Valid? */
  if(eResult != orxSTATUS_FAILURE)
  {
    orxBODY_PART_DEF            stMergedPartDef;
    orxCONST orxBODY_PART_DEF  *pstSelectedPartDef;
    orxPHYSICS_BODY_PART       *pstBodyPart;

    /* Uses part template? */
    if(orxFLAG_TEST(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_PART_TEMPLATE))
    {
      /* Has specific part definition? */
      if(_pstBodyPartDef != orxNULL)
      {
        /* Cleans merged part def */
        orxMemory_Zero(&stMergedPartDef, sizeof(orxBODY_PART_DEF));

        /* Merges template with specialized definition */
        stMergedPartDef.fFriction     = (_pstBodyPartDef->fFriction > 0.0f) ? _pstBodyPartDef->fFriction : sstBody.stBodyPartTemplate.fFriction;
        stMergedPartDef.fRestitution  = (_pstBodyPartDef->fRestitution > 0.0f) ? _pstBodyPartDef->fRestitution : sstBody.stBodyPartTemplate.fRestitution;
        stMergedPartDef.fDensity      = (_pstBodyPartDef->fDensity > 0.0f) ? _pstBodyPartDef->fDensity : sstBody.stBodyPartTemplate.fDensity;
        stMergedPartDef.u16SelfFlags  = (_pstBodyPartDef->u16SelfFlags != 0) ? _pstBodyPartDef->u16SelfFlags : sstBody.stBodyPartTemplate.u16SelfFlags;
        stMergedPartDef.u16CheckMask  = (_pstBodyPartDef->u16CheckMask != 0) ? _pstBodyPartDef->u16CheckMask : sstBody.stBodyPartTemplate.u16CheckMask;
        stMergedPartDef.u32Flags      = (_pstBodyPartDef->u32Flags != orxBODY_PART_DEF_KU32_FLAG_NONE) ? _pstBodyPartDef->u32Flags : sstBody.stBodyPartTemplate.u32Flags;

        /* Has scale? */
        if(orxVector_IsNull(&(_pstBodyPartDef->vScale)) == orxFALSE)
        {
          orxVector_Copy(&(stMergedPartDef.vScale), &(_pstBodyPartDef->vScale));
        }
        else
        {
          orxVector_Copy(&(stMergedPartDef.vScale), &(sstBody.stBodyPartTemplate.vScale));
        }

        /* Sphere? */
        if(orxFLAG_TEST(_pstBodyPartDef->u32Flags, orxBODY_PART_DEF_KU32_FLAG_SPHERE))
        {
          orxVector_Copy(&(stMergedPartDef.stSphere.vCenter), (orxVector_IsNull(&(_pstBodyPartDef->stSphere.vCenter)) == orxFALSE) ? &(_pstBodyPartDef->stSphere.vCenter) : &(sstBody.stBodyPartTemplate.stSphere.vCenter));
          stMergedPartDef.stSphere.fRadius = (_pstBodyPartDef->stSphere.fRadius > 0.0f) ? _pstBodyPartDef->stSphere.fRadius : sstBody.stBodyPartTemplate.stSphere.fRadius;
        }
        /* Box ? */
        else if(orxFLAG_TEST(_pstBodyPartDef->u32Flags, orxBODY_PART_DEF_KU32_FLAG_BOX))
        {
          orxVector_Copy(&(stMergedPartDef.stAABox.stBox.vTL), (orxVector_IsNull(&(_pstBodyPartDef->stAABox.stBox.vTL)) == orxFALSE) ? &(_pstBodyPartDef->stAABox.stBox.vTL) : &(sstBody.stBodyPartTemplate.stAABox.stBox.vTL));
          orxVector_Copy(&(stMergedPartDef.stAABox.stBox.vBR), (orxVector_IsNull(&(_pstBodyPartDef->stAABox.stBox.vBR)) == orxFALSE) ? &(_pstBodyPartDef->stAABox.stBox.vBR) : &(sstBody.stBodyPartTemplate.stAABox.stBox.vBR));
        }

        /* Selects it */
        pstSelectedPartDef = &stMergedPartDef;
      }
      else
      {
        /* Selects template */
        pstSelectedPartDef = &(sstBody.stBodyPartTemplate);
      }
    }
    else
    {
      /* Selects specialized definition */
      pstSelectedPartDef = _pstBodyPartDef;
    }

    /* Creates part */
    pstBodyPart = orxPhysics_CreateBodyPart(_pstBody->pstData, pstSelectedPartDef);

    /* Valid? */
    if(pstBodyPart != orxNULL)
    {
      /* Stores it */
      _pstBody->astPartList[_u32Index].pstData = pstBodyPart;

      /* Stores its definition */
      orxMemory_Copy(&(_pstBody->astPartList[_u32Index].stDef), pstSelectedPartDef, sizeof(orxBODY_PART_DEF));
    }
    else
    {
      /* !!! MSG !!! */

      /* Cleans reference */
      _pstBody->astPartList[_u32Index].pstData = orxNULL;

      /* Updates result */
      eResult = orxSTATUS_FAILURE;
    }
  }

  /* Done! */
  return eResult;
}

/** Adds a part to body from config
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _u32Index       Part index (should be less than orxBODY_KU32_PART_MAX_NUMBER)
 * @param[in]   _zConfigID      Body part config ID
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_AddPartFromConfig(orxBODY *_pstBody, orxU32 _u32Index, orxCONST orxSTRING _zConfigID)
{
  orxSTRING zPreviousSection;
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_u32Index < orxBODY_KU32_PART_MAX_NUMBER);
  orxASSERT((_zConfigID != orxNULL) && (*_zConfigID != *orxSTRING_EMPTY));

  /* Gets previous config section */
  zPreviousSection = orxConfig_GetCurrentSection();

  /* Selects section */
  if(orxConfig_SelectSection(_zConfigID) != orxSTATUS_FAILURE)
  {
    orxBODY_PART_DEF stBodyPartDef;

    /* Clears body part definition */
    orxMemory_Zero(&stBodyPartDef, sizeof(orxBODY_PART_DEF));

    /* Inits it */
    stBodyPartDef.fFriction     = orxConfig_GetFloat(orxBODY_KZ_CONFIG_FRICTION);
    stBodyPartDef.fRestitution  = orxConfig_GetFloat(orxBODY_KZ_CONFIG_RESTITUTION);
    stBodyPartDef.fDensity      = orxConfig_GetFloat(orxBODY_KZ_CONFIG_DENSITY);
    stBodyPartDef.u16SelfFlags  = (orxU16)orxConfig_GetU32(orxBODY_KZ_CONFIG_SELF_FLAGS);
    stBodyPartDef.u16CheckMask  = (orxU16)orxConfig_GetU32(orxBODY_KZ_CONFIG_CHECK_MASK);
    orxObject_GetScale(orxOBJECT(_pstBody->pstOwner), &(stBodyPartDef.vScale));
    if(orxConfig_GetBool(orxBODY_KZ_CONFIG_SOLID) != orxFALSE)
    {
      stBodyPartDef.u32Flags |= orxBODY_PART_DEF_KU32_FLAG_SOLID;
    }
    if(orxString_Compare(orxString_LowerCase(orxConfig_GetString(orxBODY_KZ_CONFIG_TYPE)), orxBODY_KZ_TYPE_SPHERE) == 0)
    {
      /* Updates sphere specific info */
      stBodyPartDef.u32Flags |= orxBODY_PART_DEF_KU32_FLAG_SPHERE;
      if(((orxConfig_HasValue(orxBODY_KZ_CONFIG_CENTER) == orxFALSE)
       && (orxConfig_HasValue(orxBODY_KZ_CONFIG_RADIUS) == orxFALSE))
      || (orxString_Compare(orxString_LowerCase(orxConfig_GetString(orxBODY_KZ_CONFIG_RADIUS)), orxBODY_KZ_FULL) == 0)
      || (orxString_Compare(orxString_LowerCase(orxConfig_GetString(orxBODY_KZ_CONFIG_CENTER)), orxBODY_KZ_FULL) == 0))
      {
        orxVECTOR vPivot, vSize;
        orxFLOAT  fRadius;

        /* Gets object size, scale & pivot */
        orxObject_GetSize(orxOBJECT(_pstBody->pstOwner), &vSize);
        orxObject_GetPivot(orxOBJECT(_pstBody->pstOwner), &vPivot);

        /* Gets minimal radius */
        fRadius = orx2F(0.25f) * (vSize.fX + vSize.fY);

        /* Inits body part def */
        orxVector_Set(&(stBodyPartDef.stSphere.vCenter), fRadius - vPivot.fX, fRadius - vPivot.fY, fRadius - vPivot.fZ);
        stBodyPartDef.stSphere.fRadius = fRadius;
      }
      else
      {
        orxConfig_GetVector(orxBODY_KZ_CONFIG_CENTER, &(stBodyPartDef.stSphere.vCenter));
        stBodyPartDef.stSphere.fRadius = orxConfig_GetFloat(orxBODY_KZ_CONFIG_RADIUS);
      }
    }
    else
    {
      /* Updates box specific info */
      stBodyPartDef.u32Flags |= orxBODY_PART_DEF_KU32_FLAG_BOX;
      if(((orxConfig_HasValue(orxBODY_KZ_CONFIG_TOP_LEFT) == orxFALSE)
       && (orxConfig_HasValue(orxBODY_KZ_CONFIG_BOTTOM_RIGHT) == orxFALSE))
      || (orxString_Compare(orxString_LowerCase(orxConfig_GetString(orxBODY_KZ_CONFIG_TOP_LEFT)), orxBODY_KZ_FULL) == 0)
      || (orxString_Compare(orxString_LowerCase(orxConfig_GetString(orxBODY_KZ_CONFIG_BOTTOM_RIGHT)), orxBODY_KZ_FULL) == 0))
      {
        orxVECTOR vPivot, vSize;

        /* Gets object size, scale & pivot */
        orxObject_GetSize(orxOBJECT(_pstBody->pstOwner), &vSize);
        orxObject_GetPivot(orxOBJECT(_pstBody->pstOwner), &vPivot);

        /* Inits body part def */
        orxVector_Set(&(stBodyPartDef.stAABox.stBox.vTL), -vPivot.fX, -vPivot.fY, -vPivot.fZ);
        orxVector_Set(&(stBodyPartDef.stAABox.stBox.vBR), vSize.fX - vPivot.fX, vSize.fY - vPivot.fY, vSize.fZ - vPivot.fZ);
      }
      else
      {
        orxConfig_GetVector(orxBODY_KZ_CONFIG_TOP_LEFT, &(stBodyPartDef.stAABox.stBox.vTL));
        orxConfig_GetVector(orxBODY_KZ_CONFIG_BOTTOM_RIGHT, &(stBodyPartDef.stAABox.stBox.vBR));
      }
    }

    /* Adds body part */
    eResult = orxBody_AddPart(_pstBody, _u32Index, &stBodyPartDef);

    /* Restores previous section */
    orxConfig_SelectSection(zPreviousSection);
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Gets a body part
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _u32Index       Body part index (should be less than orxBODY_KU32_DATA_MAX_NUMBER)
 * @return      orxPHYSICS_BODY_PART / orxNULL
 */
orxPHYSICS_BODY_PART *orxFASTCALL orxBody_GetPart(orxCONST orxBODY *_pstBody, orxU32 _u32Index)
{
  orxPHYSICS_BODY_PART *pstResult = orxNULL;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_u32Index < orxBODY_KU32_PART_MAX_NUMBER);

  /* Updates result */
  pstResult = _pstBody->astPartList[_u32Index].pstData;

  /* Done! */
  return pstResult;
}

/** Removes a body part
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _u32Index       Part index (should be less than orxBODY_KU32_DATA_MAX_NUMBER)
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_RemovePart(orxBODY *_pstBody, orxU32 _u32Index)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Has a part? */
  if(_pstBody->astPartList[_u32Index].pstData != orxNULL)
  {
    /* Deletes it */
    orxPhysics_DeleteBodyPart(_pstBody->astPartList[_u32Index].pstData);

    /* Deletes reference */
    _pstBody->astPartList[_u32Index].pstData = orxNULL;
  }
  else
  {
    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Sets a body template
 * @param[in]   _pstBodyTemplate  Body template to set / orxNULL to remove it
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetTemplate(orxCONST orxBODY_DEF *_pstBodyTemplate)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);

  /* Has template? */
  if(_pstBodyTemplate != orxNULL)
  {
    /* Copies template */
    orxMemory_Copy(&(sstBody.stBodyTemplate), _pstBodyTemplate, sizeof(orxBODY_DEF));

    /* Updates flags */
    orxFLAG_SET(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_TEMPLATE, orxBODY_KU32_FLAG_NONE);
  }
  else
  {
    /* Clears template */
    orxMemory_Zero(&(sstBody.stBodyTemplate), sizeof(orxBODY_DEF));

    /* Updates flags */
    orxFLAG_SET(sstBody.u32Flags, orxBODY_KU32_FLAG_NONE, orxBODY_KU32_FLAG_USE_TEMPLATE);
  }

  /* Done! */
  return eResult;
}

/** Sets a body part template
 * @param[in]   _pstBodyPartTemplate  Body part template to set / orxNULL to remove it
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetPartTemplate(orxCONST orxBODY_PART_DEF *_pstBodyPartTemplate)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);

  /* Has template? */
  if(_pstBodyPartTemplate != orxNULL)
  {
    /* Copies template */
    orxMemory_Copy(&(sstBody.stBodyPartTemplate), _pstBodyPartTemplate, sizeof(orxBODY_PART_DEF));

    /* Updates flags */
    orxFLAG_SET(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_PART_TEMPLATE, orxBODY_KU32_FLAG_NONE);
  }
  else
  {
    /* Clears template */
    orxMemory_Zero(&(sstBody.stBodyPartTemplate), sizeof(orxBODY_PART_DEF));

    /* Updates flags */
    orxFLAG_SET(sstBody.u32Flags, orxBODY_KU32_FLAG_NONE, orxBODY_KU32_FLAG_USE_PART_TEMPLATE);
  }

  /* Done! */
  return eResult;
}

/** Gets the body template
 * @param[out]  _pstBodyTemplate  Body template to get
 * @return      orxBODY_DEF / orxNULL
 */
orxBODY_DEF *orxFASTCALL orxBody_GetTemplate(orxBODY_DEF *_pstBodyTemplate)
{
  orxBODY_DEF *pstResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxASSERT(_pstBodyTemplate != orxNULL);

  /* Has template? */
  if(orxFLAG_TEST(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_TEMPLATE))
  {
    /* Copies template */
    orxMemory_Copy(_pstBodyTemplate, &(sstBody.stBodyTemplate),sizeof(orxBODY_DEF));

    /* Updates result */
    pstResult = _pstBodyTemplate;
  }
  else
  {
    /* Clears template */
    orxMemory_Zero(_pstBodyTemplate, sizeof(orxBODY_DEF));

    /* Updates result */
    pstResult = orxNULL;
  }

  /* Done! */
  return pstResult;
}

/** Gets the body part template
 * @param[out]  _pstBodyPartTemplate  Body part template to get
 * @return      orxBODY_PART_DEF / orxNULL
 */
orxBODY_PART_DEF *orxFASTCALL orxBody_GetPartTemplate(orxBODY_PART_DEF *_pstBodyPartTemplate)
{
  orxBODY_PART_DEF *pstResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxASSERT(_pstBodyPartTemplate != orxNULL);

  /* Has template? */
  if(orxFLAG_TEST(sstBody.u32Flags, orxBODY_KU32_FLAG_USE_PART_TEMPLATE))
  {
    /* Copies template */
    orxMemory_Copy(_pstBodyPartTemplate, &(sstBody.stBodyPartTemplate),sizeof(orxBODY_PART_DEF));

    /* Updates result */
    pstResult = _pstBodyPartTemplate;
  }
  else
  {
    /* Clears template */
    orxMemory_Zero(_pstBodyPartTemplate, sizeof(orxBODY_PART_DEF));

    /* Updates result */
    pstResult = orxNULL;
  }

  /* Done! */
  return pstResult;
}

/** Sets a body position
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _pvPosition     Position to set
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetPosition(orxBODY *_pstBody, orxCONST orxVECTOR *_pvPosition)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvPosition != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates its position */
    eResult = orxPhysics_SetPosition(_pstBody->pstData, _pvPosition);
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Sets a body rotation
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _fRotation      Rotation to set
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetRotation(orxBODY *_pstBody, orxFLOAT _fRotation)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates its position */
    eResult = orxPhysics_SetRotation(_pstBody->pstData, _fRotation);
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Sets a body scale
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _pvScale        Scale to set
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetScale(orxBODY *_pstBody, orxCONST orxVECTOR *_pvScale)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvScale != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    orxU32 i;

    /* for all parts */
    for(i = 0; i < orxBODY_KU32_PART_MAX_NUMBER; i++)
    {
      /* Has part? */
      if(_pstBody->astPartList[i].pstData != orxNULL)
      {
        orxVECTOR vDiff;

        /* Gets diff vector */
        orxVector_Sub(&vDiff, &(_pstBody->astPartList[i].stDef.vScale), _pvScale);

        /* Is new scale different? */
        if(orxVector_IsNull(&vDiff) == orxFALSE)
        {
          /* Removes parts */
          orxBody_RemovePart(_pstBody, i);

          /* Updates its scale */
          orxVector_Copy(&(_pstBody->astPartList[i].stDef.vScale), _pvScale);

          /* Creates new part */
          _pstBody->astPartList[i].pstData = orxPhysics_CreateBodyPart(_pstBody->pstData, &(_pstBody->astPartList[i].stDef));
        }
      }
      else
      {
        break;
      }
    }

    /* Updates result */
    eResult = orxSTATUS_SUCCESS;
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Sets a body speed
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _pvSpeed        Speed to set
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetSpeed(orxBODY *_pstBody, orxCONST orxVECTOR *_pvSpeed)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvSpeed != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates its speed */
    eResult = orxPhysics_SetSpeed(_pstBody->pstData, _pvSpeed);
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Sets a body angular velocity
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _fVelocity      Angular velocity to set
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_SetAngularVelocity(orxBODY *_pstBody, orxFLOAT _fVelocity)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates its position */
    eResult = orxPhysics_SetAngularVelocity(_pstBody->pstData, _fVelocity);
  }
  else
  {
    /* !!! MSG !!! */

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Gets a body position
 * @param[in]   _pstBody        Concerned body
 * @param[out]  _pvPosition     Position to get
 * @return      Body position / orxNULL
 */
orxVECTOR *orxFASTCALL orxBody_GetPosition(orxBODY *_pstBody, orxVECTOR *_pvPosition)
{
  orxVECTOR *pvResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvPosition != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates result */
    pvResult = orxPhysics_GetPosition(_pstBody->pstData, _pvPosition);
  }
  else
  {
    /* Updates result */
    pvResult = orxNULL;
  }

  /* Done! */
  return pvResult;
}

/** Gets a body rotation
 * @param[in]   _pstBody        Concerned body
 * @return      Body rotation
 */
orxFLOAT orxFASTCALL orxBody_GetRotation(orxBODY *_pstBody)
{
  orxFLOAT fResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates result */
    fResult = orxPhysics_GetRotation(_pstBody->pstData);
  }
  else
  {
    /* Updates result */
    fResult = orxFLOAT_0;
  }

  /* Done! */
  return fResult;
}

/** Gets a body speed
 * @param[in]   _pstBody        Concerned body
 * @param[out]   _pvSpeed       Speed to get
 * @return      Body speed / orxNULL
 */
orxVECTOR *orxFASTCALL orxBody_GetSpeed(orxBODY *_pstBody, orxVECTOR *_pvSpeed)
{
  orxVECTOR *pvResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvSpeed != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates result */
    pvResult = orxPhysics_GetSpeed(_pstBody->pstData, _pvSpeed);
  }
  else
  {
    /* Updates result */
    pvResult = orxNULL;
  }

  /* Done! */
  return pvResult;
}

/** Gets a body angular velocity
 * @param[in]   _pstBody        Concerned body
 * @return      Body angular velocity
 */
orxFLOAT orxFASTCALL orxBody_GetAngularVelocity(orxBODY *_pstBody)
{
  orxFLOAT fResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Updates result */
    fResult = orxPhysics_GetAngularVelocity(_pstBody->pstData);
  }
  else
  {
    /* Updates result */
    fResult = orxFLOAT_0;
  }

  /* Done! */
  return fResult;
}

/** Gets a body center of mass
 * @param[in]   _pstBody        Concerned body
 * @param[out]  _pvMassCenter   Mass center to get
 * @return      Mass center / orxNULL
 */
orxVECTOR *orxFASTCALL orxBody_GetMassCenter(orxBODY *_pstBody, orxVECTOR *_pvMassCenter)
{
  orxVECTOR *pvResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvMassCenter != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Gets mass center */
    pvResult = orxPhysics_GetMassCenter(_pstBody->pstData, _pvMassCenter);
  }
  else
  {
    /* Updates result */
    pvResult = orxNULL;
  }

  /* Done! */
  return pvResult;
}

/** Applies a torque
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _fTorque        Torque to apply
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_ApplyTorque(orxBODY *_pstBody, orxFLOAT _fTorque)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Applies torque */
    eResult = orxPhysics_ApplyTorque(_pstBody->pstData, _fTorque);
  }
  else
  {
    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Applies a force
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _pvForce        Force to apply
 * @param[in]   _pvPoint        Point (world coordinates) where the force will be applied, if orxNULL, center of mass will be used
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_ApplyForce(orxBODY *_pstBody, orxCONST orxVECTOR *_pvForce, orxCONST orxVECTOR *_pvPoint)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvForce != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Has given point? */
    if(_pvPoint != orxNULL)
    {
      /* Applies force */
      eResult = orxPhysics_ApplyForce(_pstBody->pstData, _pvForce, _pvPoint);
    }
    else
    {
      orxVECTOR vMassCenter;

      /* Applies force on mass center */
      eResult = orxPhysics_ApplyForce(_pstBody->pstData, _pvForce, orxPhysics_GetMassCenter(_pstBody->pstData, &vMassCenter));
    }
  }
  else
  {
    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Applies an impulse
 * @param[in]   _pstBody        Concerned body
 * @param[in]   _pvImpulse      Impulse to apply
 * @param[in]   _pvPoint        Point (world coordinates) where the impulse will be applied, if orxNULL, center of mass will be used
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxBody_ApplyImpulse(orxBODY *_pstBody, orxCONST orxVECTOR *_pvImpulse, orxCONST orxVECTOR *_pvPoint)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstBody.u32Flags & orxBODY_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstBody);
  orxASSERT(_pvImpulse != orxNULL);

  /* Has data? */
  if(orxStructure_TestFlags(_pstBody, orxBODY_KU32_FLAG_HAS_DATA))
  {
    /* Has given point? */
    if(_pvPoint != orxNULL)
    {
      /* Applies impulse */
      eResult = orxPhysics_ApplyImpulse(_pstBody->pstData, _pvImpulse, _pvPoint);
    }
    else
    {
      orxVECTOR vMassCenter;

      /* Applies impusle on mass center */
      eResult = orxPhysics_ApplyForce(_pstBody->pstData, _pvImpulse, orxPhysics_GetMassCenter(_pstBody->pstData, &vMassCenter));
    }
  }
  else
  {
    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}
