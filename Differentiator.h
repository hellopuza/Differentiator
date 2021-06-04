/*------------------------------------------------------------------------------
    * File:        Differentiator.h                                            *
    * Description: Declaration of functions and data types used for            *
    *              calculating derivatives of functions                        *
    * Created:     15 may 2021                                                 *
    * Author:      Artem Puzankov                                              *
    * Email:       puzankov.ao@phystech.edu                                    *
    * GitHub:      https://github.com/hellopuza                                *
    * Copyright © 2021 Artem Puzankov. All rights reserved.                    *
    *///------------------------------------------------------------------------

#ifndef DIFFERENTIATOR_H_INCLUDED
#define DIFFERENTIATOR_H_INCLUDED

#define _CRT_SECURE_NO_WARNINGS
//#define NDEBUG


#if defined (__GNUC__) || defined (__clang__) || defined (__clang_major__)
    #define __FUNC_NAME__   __PRETTY_FUNCTION__

#elif defined (_MSC_VER)
    #define __FUNC_NAME__   __FUNCSIG__

#else
    #define __FUNC_NAME__   __FUNCTION__

#endif


#include "Calculator/Calculator.h"


//==============================================================================
/*------------------------------------------------------------------------------
                   Differentiator errors                                       *
*///----------------------------------------------------------------------------
//==============================================================================


enum DifferentiatorErrors
{
    DIFF_NOT_OK = -1                                                       ,
    DIFF_OK = 0                                                            ,
    DIFF_NO_MEMORY                                                         ,

    DIFF_DESTRUCTED                                                        ,
    DIFF_INCORRECT_INPUT_SYNTAX_BASE                                       ,
    DIFF_NULL_INPUT_DIFFERENTIATOR_PTR                                     ,
    DIFF_NULL_INPUT_FILENAME                                               ,
    DIFF_WRONG_SYNTAX_TREE_LEAF                                            ,
    DIFF_WRONG_SYNTAX_TREE_NODE                                            ,
    DIFF_WRONG_TREE_ONE_CHILD                                              ,
};

static const char* diff_errstr[] =
{
    "ERROR"                                                                ,
    "OK"                                                                   ,
    "Failed to allocate memory"                                            ,

    "Differentiator has already destructed"                                ,
    "Incorrect input syntax base"                                          ,
    "The input value of the differentiator pointer turned out to be zero"  ,
    "The input value of the differentiator filename turned out to be zero" ,
    "Wrohg syntax tree leaf"                                               ,
    "Wrohg syntax tree node"                                               ,
    "Every node must have 0 or 2 children"                                 ,
};

static const char* DIFFERENTIATOR_LOGNAME = "differentiator.log";

#define BASE_CHECK if (tree_.Check ())                                                                    \
                   {                                                                                      \
                     tree_.Dump();                                                                        \
                     tree_.PrintError (TREE_LOGNAME , __FILE__, __LINE__, __FUNC_NAME__, tree_.errCode_); \
                     exit(tree_.errCode_);                                                                \
                   }                                                                                      \
                   if (checkBase (tree_.root_))                                                           \
                   {                                                                                      \
                     DIFF_ASSERTOK(state_, state_);                                                       \
                   } //

#define DIFF_ASSERTOK(cond, err) if (cond)                                                                    \
                                {                                                                             \
                                  PrintError(DIFFERENTIATOR_LOGNAME, __FILE__, __LINE__, __FUNC_NAME__, err); \
                                  tree_.Dump();                                                               \
                                  exit(err);                                                                  \
                                } //


//==============================================================================
/*------------------------------------------------------------------------------
                   Differentiator constants and types                          *
*///----------------------------------------------------------------------------
//==============================================================================

#define PREV_CONNECT(old_node, new_node)                                                \
        if (old_node->prev_ != nullptr)                                                 \
        {                                                                               \
            if (old_node->prev_->left_ == old_node) old_node->prev_->left_  = new_node; \
            else                                    old_node->prev_->right_ = new_node; \
        }                                                                               \
        else tree_.root_ = new_node;                                                    \
                                                                                        \
        new_node->prev_ = old_node->prev_;                                              \
                                                                                        \
        new_node->recountPrev();                                                        \
        new_node->recountDepth(); //


class Differentiator
{
private:

    int state_;
    char* filename_;

    Variable           diff_var_ = {0, "x"};
    Tree<CalcNodeData> tree_;
    Stack<Variable>    constants_;


    Stack<char*> path2badnode_;

public:

//------------------------------------------------------------------------------
/*! @brief   Differentiator default constructor.
*/

    Differentiator ();

//------------------------------------------------------------------------------
/*! @brief   Differentiator constructor.
 *
 *  @param   filename    Name of input file
 */

    Differentiator (char* filename);

//------------------------------------------------------------------------------
/*! @brief   Differentiator copy constructor (deleted).
 *
 *  @param   obj         Source differentiator
 */

    Differentiator (const Differentiator& obj);

    Differentiator& operator = (const Differentiator& obj); // deleted

//------------------------------------------------------------------------------
/*! @brief   Differentiator destructor.
 */

   ~Differentiator ();

//------------------------------------------------------------------------------
/*! @brief   Differentiating process.
 *
 *  @return  error code
 */

    int Run ();

/*------------------------------------------------------------------------------
                   Private functions                                           *
*///----------------------------------------------------------------------------

private:

//------------------------------------------------------------------------------
/*! @brief   Differentiating process.
 *
 *  @param   node_cur    Current node
 *
 *  @return  error code
 */

    int Differentiate (Node<CalcNodeData>* node_cur);

//------------------------------------------------------------------------------
/*! @brief   Write derivative to console or to file.
 *
 *  @return  error code
 */

    void Write ();

//------------------------------------------------------------------------------
/*! @brief   Prints an error wih description to the console and to the log file.
 *
 *  @param   logname     Name of the log file
 *  @param   file        Name of the program file
 *  @param   line        Number of line with an error
 *  @param   function    Name of the function with an error
 *  @param   err         Error code
 */

    void PrintError (const char* logname, const char* file, int line, const char* function, int err);

//------------------------------------------------------------------------------
};

//------------------------------------------------------------------------------

#endif // DIFFERENTIATOR_H_INCLUDED
