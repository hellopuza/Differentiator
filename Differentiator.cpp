/*------------------------------------------------------------------------------
    * File:        Differentiator.cpp                                          *
    * Description: Functions for calculating derivatives of functions.         *
    * Created:     15 may 2021                                                 *
    * Author:      Artem Puzankov                                              *
    * Email:       puzankov.ao@phystech.edu                                    *
    * GitHub:      https://github.com/hellopuza                                *
    * Copyright © 2021 Artem Puzankov. All rights reserved.                    *
    *///------------------------------------------------------------------------

#include "Differentiator.h"

//------------------------------------------------------------------------------

Differentiator::Differentiator () :
    filename_     (nullptr),
    tree_         ((char*)"expression"),
    path2badnode_ ((char*)"path2badnode_"),
    state_        (DIFF_OK)
{}

//------------------------------------------------------------------------------

Differentiator::Differentiator (char* filename) :
    filename_     (filename),
    tree_         (GetTrueFileName(filename)),
    path2badnode_ ((char*)"path2badnode_"),
    state_        (DIFF_OK)
{}

//------------------------------------------------------------------------------

Differentiator::~Differentiator ()
{
    DIFF_ASSERTOK((this == nullptr),           DIFF_NULL_INPUT_DIFFERENTIATOR_PTR);
    DIFF_ASSERTOK((state_ == DIFF_DESTRUCTED), DIFF_DESTRUCTED                   );

    filename_ = nullptr;

    state_ = DIFF_DESTRUCTED;
}

//------------------------------------------------------------------------------

int Differentiator::Run ()
{
    DIFF_ASSERTOK((this == nullptr), DIFF_NULL_INPUT_DIFFERENTIATOR_PTR);

    if (filename_ == nullptr)
    {
        bool running = true;
        while (running)
        {
            printf("\nEnter expression: ");

            char* expr = ScanExpr();
            Expression expression = { expr, expr };

            int err = Expr2Tree(expression, tree_);
            delete [] expr;
            if (!err)
            {
                tree_.Dump();

                Differentiate(tree_.root_);
                Write();
            }
            tree_.Clean();

            printf("Continue [Y/n]? ");
            running = scanAns();
        }
    }
    else
    {
        Text text(filename_);
        char* expr = text.text_;
        Expression expression = { expr, expr };

        int err = Expr2Tree(expression, tree_);
        delete [] expr;
        if (err) return err;

        tree_.Dump();

        Differentiate(tree_.root_);
        Write();
    }
    
    return DIFF_OK;
}

//------------------------------------------------------------------------------

int Differentiator::Differentiate (Node<CalcNodeData>* node_cur)
{
    switch (node_cur->getData().node_type)
    {
    case NODE_FUNCTION:
    case NODE_OPERATOR:

        switch (node_cur->getData().op.code)
        {
        case OP_MUL:
            break;
        case OP_ADD:
            break;
        case OP_DIV:
            break;
        case OP_SUB:
            break;
        case OP_POW:
            break;
        case OP_ARCCOS:
            break;
        case OP_ARCCOSH:
            break;
        case OP_ARCCOT:
            break;
        case OP_ARCCOTH:
            break;
        case OP_ARCSIN:
            break;
        case OP_ARCSINH:
            break;
        case OP_ARCTAN:
            break;
        case OP_ARCTANH:
            break;
        case OP_CBRT:
            break;
        case OP_COS:
            break;
        case OP_COSH:
            break;
        case OP_COT:
            break;
        case OP_COTH:
            break;
        case OP_EXP:
            break;
        case OP_LG:
            break;
        case OP_LN:
            break;
        case OP_SIN:
            break;
        case OP_SINH:
            break;
        case OP_SQRT:
            break;
        case OP_TAN:
            break;
        case OP_TANH:
            break;
        default: assert(0);
        }
        break;

    case NODE_VARIABLE:
        break;

    case NODE_NUMBER:
        break;

    default: assert(0);
    }

    return DIFF_OK;
}

//------------------------------------------------------------------------------

void Differentiator::Write ()
{
    Expression expr = {};
    Tree2Expr(tree_, expr);

    if (filename_ == nullptr)
    {
        printf("result: %s\n", expr.str);
    }
    else
    {
        FILE* output = fopen(filename_, "w");
        assert(output != nullptr);

        fprintf(output, "%s", expr.str);
        fclose(output);
    }
}

//------------------------------------------------------------------------------

void Differentiator::PrintError (const char* logname, const char* file, int line, const char* function, int err)
{
    assert(function != nullptr);
    assert(logname  != nullptr);
    assert(file     != nullptr);

    FILE* log = fopen(logname, "a");
    assert(log != nullptr);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(log, "###############################################################################\n");
    fprintf(log, "TIME: %d-%02d-%02d %02d:%02d:%02d\n\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(log, "ERROR: file %s  line %d  function %s\n\n", file, line, function);
    fprintf(log, "%s\n", diff_errstr[err + 1]);

    if (path2badnode_.getSize() != 0)
    {
        fprintf(log, "%s", path2badnode_.getName());
        for (int i = path2badnode_.getSize() - 1; i > -1; --i)
            fprintf(log, " -> [%s]", path2badnode_[i]);

        fprintf(log, "\n");
    }
    fprintf(log, "You can look tree dump in %s\n", DUMP_PICT_NAME);
    fclose(log);

    ////

    printf (     "ERROR: file %s  line %d  function %s\n",   file, line, function);
    printf (     "%s\n\n", diff_errstr[err + 1]);

    if (path2badnode_.getSize() != 0)
    {
        printf("%s", path2badnode_.getName());
        for (int i = path2badnode_.getSize() - 1; i > -1; --i)
            printf(" -> [%s]", path2badnode_[i]);

        printf("\n");
    }
    printf (     "You can look tree dump in %s\n", DUMP_PICT_NAME);
}

//------------------------------------------------------------------------------
