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
 * @file orxTree.c
 * @date 07/04/2005
 * @author iarwain@orx-project.org
 *
 */


#include "utils/orxTree.h"
#include "memory/orxMemory.h"


/***************************************************************************
 * Structure declaration                                                   *
 ***************************************************************************/

/***************************************************************************
 * Static variables                                                        *
 ***************************************************************************/

/***************************************************************************
 * Private functions                                                       *
 ***************************************************************************/

/** Removes a node from its tree
 * @param[in]   _pstNode                        Concerned node
 * @param[in]   _bBranchRemove                  Remove the whole branch or only the single node
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
static orxSTATUS orxFASTCALL orxTree_PrivateRemove(orxTREE_NODE *_pstNode, orxBOOL _bBranchRemove)
{
  orxTREE  *pstTree;
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(_pstNode != orxNULL);

  /* Gets tree */
  pstTree = _pstNode->pstTree;

  /* Remove the whole branch? */
  if(_bBranchRemove != orxFALSE)
  {
    /* Isn't root? */
    if(pstTree->pstRoot != _pstNode)
    {
      /* Was firt child? */
      if(_pstNode->pstParent->pstChild == _pstNode)
      {
        /* Udpates parent */
        _pstNode->pstParent->pstChild = _pstNode->pstSibling;
      }
      else
      {
        orxTREE_NODE *pstChild;

        /* Finds left sibling */
        for(pstChild = _pstNode->pstParent->pstChild;
            pstChild->pstSibling != _pstNode;
            pstChild = pstChild->pstSibling);

        /* Updates it */
        pstChild->pstSibling = _pstNode->pstSibling;
      }

      /* Updates node */
      _pstNode->pstParent   = orxNULL;
      _pstNode->pstSibling  = orxNULL;
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Node isn't root.");

      /* Can't process */
      eResult = orxSTATUS_FAILURE;
    }
  }
  /* Remove completely from tree */
  else
  {
    /* Is root? */
    if(pstTree->pstRoot == _pstNode)
    {
      /* Is the last node in tree? */
      if(pstTree->u32Counter == 1)
      {
        /* Removes it */
        pstTree->pstRoot = orxNULL;

        /* Updates node */
        orxMemory_Zero(_pstNode, sizeof(orxTREE_NODE));

        /* Updates counter */
        pstTree->u32Counter = 0;
      }
      else
      {
        /* Logs message */
        orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Node is not last one in tree.");

        /* Can't process */
        eResult = orxSTATUS_FAILURE;
      }
    }
    else
    {
      orxTREE_NODE *pstNewChild;

      /* Had child? */
      if(_pstNode->pstChild != orxNULL)
      {
        orxTREE_NODE *pstChild;

        /* Updates all children but last */
        for(pstChild = _pstNode->pstChild;
            pstChild->pstSibling != orxNULL;
            pstChild = pstChild->pstSibling)
        {
          /* Updates it */
          pstChild->pstParent = _pstNode->pstParent;
        }

        /* Updates last child */
        pstChild->pstParent   = _pstNode->pstParent;
        pstChild->pstSibling  = _pstNode->pstSibling;

        /* New parent's child is previous first child */
        pstNewChild = _pstNode->pstChild;
      }
      /* No child */
      else
      {
        /* New parent's child is previous sibling */
        pstNewChild = _pstNode->pstSibling;
      }

      /* Was first child? */
      if(_pstNode->pstParent->pstChild == _pstNode)
      {
        /* Updates parent */
        _pstNode->pstParent->pstChild = pstNewChild;
      }
      /* Not first child */
      else
      {
        orxTREE_NODE *pstChild;

        /* Find left sibling */
        for(pstChild = _pstNode->pstParent->pstChild;
            pstChild->pstSibling != _pstNode;
            pstChild = pstChild->pstSibling);

        /* Updates it */
        pstChild->pstSibling = pstNewChild;
      }

      /* Updates node */
      orxMemory_Zero(_pstNode, sizeof(orxTREE_NODE));

      /* Updates counter */
      pstTree->u32Counter--;
    }
  }

  /* Done! */
  return eResult;
}


/***************************************************************************
 * Public functions                                                        *
 ***************************************************************************/

/** Cleans a tree
 * @param[in]   _pstTree                        Concerned tree
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_Clean(orxTREE *_pstTree)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(_pstTree != orxNULL);

  /* Non empty? */
  while((_pstTree->u32Counter > 1) && (eResult != orxSTATUS_FAILURE))
  {
    /* Removes root node */
    eResult = orxTree_Remove(_pstTree->pstRoot->pstChild);
  }

  /* Successful? */
  if(eResult != orxSTATUS_FAILURE)
  {
    /* Removes root */
    orxTree_Remove(_pstTree->pstRoot);

    /* Cleans tree */
    orxMemory_Zero(_pstTree, sizeof(orxTREE));
  }

  /* Done! */
  return eResult;
}

/** Adds a node at the root of a tree
 * @param[in]   _pstTree                        Concerned tree
 * @param[in]   _pstNode                        Node to add
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_AddRoot(orxTREE *_pstTree, orxTREE_NODE *_pstNode)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(_pstTree != orxNULL);
  orxASSERT(_pstNode != orxNULL);

  /* Isn't already in a tree? */
  if(_pstNode->pstTree == orxNULL)
  {
    /* Has a root? */
    if(_pstTree->pstRoot != orxNULL)
    {
      /* Adds as parent of the current root */
      eResult = orxTree_AddParent(_pstTree->pstRoot, _pstNode);
    }
    else
    {
      /* Checks there are no node right now */
      orxASSERT(_pstTree->u32Counter == 0);

      /* Stores it as root */
      _pstTree->pstRoot = _pstNode;

      /* Cleans it */
      orxMemory_Zero(_pstNode, sizeof(orxTREE_NODE));

      /* Stores tree pointer */
      _pstNode->pstTree = _pstTree;

      /* Updates counter */
      _pstTree->u32Counter++;
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Node is already in a tree, remove it from there first.");

    /* Already in a tree */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Adds a node as a parent of another one
 * @param[in]   _pstRefNode                     Reference node (add as a parent of this one)
 * @param[in]   _pstNode                        Node to add
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_AddParent(orxTREE_NODE *_pstRefNode, orxTREE_NODE *_pstNode)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;
  orxTREE  *pstTree;

  /* Checks */
  orxASSERT(_pstRefNode != orxNULL);
  orxASSERT(_pstNode != orxNULL);

  /* Isn't already in a tree? */
  if(_pstNode->pstTree == orxNULL)
  {
    /* Gets tree */
    pstTree = _pstRefNode->pstTree;

    /* Valid? */
    if(pstTree != orxNULL)
    {
      /* Adds it in the tree */
      _pstNode->pstChild    = _pstRefNode;
      _pstNode->pstParent   = _pstRefNode->pstParent;
      _pstNode->pstTree     = pstTree;
      _pstNode->pstSibling  = _pstRefNode->pstSibling;

      /* Updates parent? */
      if(_pstRefNode->pstParent != orxNULL)
      {
        /* Was first child? */
        if(_pstRefNode->pstParent->pstChild == _pstRefNode)
        {
            /* Updates parent */
            _pstRefNode->pstParent->pstChild = _pstNode;
        }
        else
        {
          orxTREE_NODE *pstChild;

          /* Finds left sibling */
          for(pstChild = _pstRefNode->pstParent->pstChild;
              pstChild->pstSibling != _pstRefNode;
              pstChild = pstChild->pstSibling);

          /* Updates sibling */
          pstChild->pstSibling = _pstNode;
        }
      }
      else
      {
        /* Checks node was the root */
        orxASSERT(pstTree->pstRoot == _pstRefNode);

        /* Updates new root */
        pstTree->pstRoot = _pstNode;
      }

      /* Updates ref node */
      _pstRefNode->pstParent  = _pstNode;
      _pstRefNode->pstSibling = orxNULL;

      /* Updates counter */
      pstTree->u32Counter++;
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Parent node's tree is invalid.");

      /* No tree found */
      eResult = orxSTATUS_FAILURE;
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Node is already in a tree, use MoveAsChild instead.");

    /* Already in a tree */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Adds a node as a sibling of another one
 * @param[in]   _pstRefNode                     Reference node (add as a sibling of this one)
 * @param[in]   _pstNode                        Node to add
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_AddSibling(orxTREE_NODE *_pstRefNode, orxTREE_NODE *_pstNode)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;
  orxTREE  *pstTree;

  /* Checks */
  orxASSERT(_pstRefNode != orxNULL);
  orxASSERT(_pstNode != orxNULL);

  /* Isn't already in a tree? */
  if(_pstNode->pstTree == orxNULL)
  {
    /* Gets tree */
    pstTree = _pstRefNode->pstTree;

    /* Valid? */
    if(pstTree != orxNULL)
    {
      /* Isn't the root? */
      if(pstTree->pstRoot != _pstRefNode)
      {
        /* Checks */
        orxASSERT(_pstRefNode->pstParent != orxNULL);

        /* Adds it in the tree */
        _pstNode->pstParent   = _pstRefNode->pstParent;
        _pstNode->pstSibling  = _pstRefNode->pstSibling;
        _pstNode->pstTree     = pstTree;
        _pstNode->pstChild    = orxNULL;

        /* Updates ref node */
        _pstRefNode->pstSibling = _pstNode;

        /* Updates counter */
        pstTree->u32Counter++;
      }
      else
      {
        /* Logs message */
        orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Can't add a node as a sibling of the root node.");

        /* Updates result */
        eResult = orxSTATUS_FAILURE;
      }
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Destination's tree is invalid.");

      /* No tree found */
      eResult = orxSTATUS_FAILURE;
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Node is already in a tree, use MoveAsChild instead.");

    /* Already in a tree */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Adds a node as a child of another one
 * @param[in]   _pstRefNode                     Reference node (add as a child of this one)
 * @param[in]   _pstNode                        Node to add
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_AddChild(orxTREE_NODE *_pstRefNode, orxTREE_NODE *_pstNode)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;
  orxTREE  *pstTree;

  /* Checks */
  orxASSERT(_pstRefNode != orxNULL);
  orxASSERT(_pstNode != orxNULL);

  /* Isn't already in a tree? */
  if(_pstNode->pstTree == orxNULL)
  {
    /* Gets tree */
    pstTree = _pstRefNode->pstTree;

    /* Valid? */
    if(pstTree != orxNULL)
    {
      /* Adds it in the tree */
      _pstNode->pstParent   = _pstRefNode;
      _pstNode->pstSibling  = _pstRefNode->pstChild;
      _pstNode->pstTree     = pstTree;
      _pstNode->pstChild    = orxNULL;

      /* Updates ref node */
      _pstRefNode->pstChild = _pstNode;

      /* Updates counter */
      pstTree->u32Counter++;
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Destination's tree is invalid.");

      /* No tree found */
      eResult = orxSTATUS_FAILURE;
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Node is already in a tree, use MoveAsChild instead.");

    /* Already in a tree */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Moves a node as a child of another one of the same tree
 * @param[in]   _pstRefNode                     Reference node (move as a child of this one)
 * @param[in]   _pstNode                        Node to move
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_MoveAsChild(orxTREE_NODE *_pstRefNode, orxTREE_NODE *_pstNode)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(_pstRefNode != orxNULL);
  orxASSERT(_pstNode != orxNULL);

  /* Is already in the same tree? */
  if(_pstNode->pstTree == _pstRefNode->pstTree)
  {
    orxTREE_NODE *pstTest;

    /* Checks for preventing tree from turning into graph */
    for(pstTest = _pstRefNode;
        (pstTest != orxNULL) && (pstTest != _pstNode);
        pstTest = pstTest->pstParent);

    /* No graph cycle found? */
    if(pstTest == orxNULL)
    {
      /* Removes it from its place */
      eResult = orxTree_PrivateRemove(_pstNode, orxTRUE);

      /* Success? */
      if(eResult != orxSTATUS_FAILURE)
      {
        /* Adds it at new place */
        _pstNode->pstParent   = _pstRefNode;
        _pstNode->pstSibling  = _pstRefNode->pstChild;

        /* Updates ref node */
        _pstRefNode->pstChild = _pstNode;
      }
      else
      {
        /* Logs message */
        orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Failed to remove node from current tree when attempting to move.");
      }
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Graph cycle found, invalid move.");

      /* Can't process */
      eResult = orxSTATUS_FAILURE;
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Attempted to move node between two different trees.");

    /* Not already in the tree */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Removes a node from its tree
 * @param[in]   _pstNode                        Concerned node
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTree_Remove(orxTREE_NODE *_pstNode)
{
  orxTREE  *pstTree;
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(_pstNode != orxNULL);

  /* Gets tree */
  pstTree = _pstNode->pstTree;

  /* Is in a tree? */
  if(pstTree != orxNULL)
  {
    /* Checks tree is non empty */
    orxASSERT(pstTree->u32Counter != 0);

    /* Removes it */
    eResult = orxTree_PrivateRemove(_pstNode, orxFALSE);
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_SYSTEM, "Referenced object for removing node is not a tree.");

    /* Not in a tree */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}
