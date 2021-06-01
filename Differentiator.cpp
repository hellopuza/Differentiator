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
    int err = DIFF_OK;

    switch (node_cur->getData().node_type)
    {
    case NODE_FUNCTION:
    case NODE_OPERATOR:

        switch (node_cur->getData().op.code)
        {
        case OP_ADD:
        case OP_SUB:
        {
            Node<CalcNodeData>* Sum   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Sum_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* Sum_r = new Node<CalcNodeData>;

            Sum ->setData({op_names[node_cur->getData().op.code], NODE_OPERATOR});

            Sum->left_  = Sum_l;
            Sum->right_ = Sum_r;

            if (node_cur->prev_ != nullptr)
            {
                if (node_cur->prev_->left_ == node_cur) node_cur->prev_->left_  = Sum;
                else                                    node_cur->prev_->right_ = Sum;
            }

            Sum->prev_ = node_cur->prev_;

            Sum->recountPrev();
            Sum->recountDepth();

            *Sum_l = *node_cur->left_;
            *Sum_r = *node_cur->right_;

            err = Differentiate(Sum_l);
            if (err) return err;

            err = Differentiate(Sum_r);
            if (err) return err;

            break;
        }
        case OP_MUL:
        {
            Node<CalcNodeData>* Sum  = new Node<CalcNodeData>;
            Node<CalcNodeData>* lMul = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul = new Node<CalcNodeData>;

            Node<CalcNodeData>* lMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* lMul_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul_r = new Node<CalcNodeData>;

            Sum ->setData({op_names[OP_ADD], NODE_OPERATOR});
            lMul->setData({op_names[OP_MUL], NODE_OPERATOR});
            rMul->setData({op_names[OP_MUL], NODE_OPERATOR});

            Sum->left_  = lMul;
            Sum->right_ = rMul;

            lMul->left_  = lMul_l;
            lMul->right_ = lMul_r;
            
            rMul->left_  = rMul_l;
            rMul->right_ = rMul_r;

            if (node_cur->prev_ != nullptr)
            {
                if (node_cur->prev_->left_ == node_cur) node_cur->prev_->left_  = Sum;
                else                                    node_cur->prev_->right_ = Sum;
            }

            Sum->prev_ = node_cur->prev_;

            Sum->recountPrev();
            Sum->recountDepth();

            *lMul_l = *node_cur->left_;
            *lMul_r = *node_cur->right_;

            *rMul_l = *node_cur->left_;
            *rMul_r = *node_cur->right_;

            err = Differentiate(lMul_l);
            if (err) return err;

            err = Differentiate(rMul_r);
            if (err) return err;

            break;
        }
        case OP_DIV:
        {
            Node<CalcNodeData>* Div  = new Node<CalcNodeData>;
            Node<CalcNodeData>* lSub = new Node<CalcNodeData>;
            Node<CalcNodeData>* rPow = new Node<CalcNodeData>;

            Node<CalcNodeData>* rPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rPow_r = new Node<CalcNodeData>;

            Div ->setData({op_names[OP_DIV], NODE_OPERATOR});
            rPow->setData({op_names[OP_POW], NODE_OPERATOR});

            Div->left_  = lSub;
            Div->right_ = rPow;

            rPow->left_  = rPow_l;
            rPow->right_ = rPow_r;

            if (node_cur->prev_ != nullptr)
            {
                if (node_cur->prev_->left_ == node_cur) node_cur->prev_->left_  = Div;
                else                                    node_cur->prev_->right_ = Div;
            }

            Div->prev_ = node_cur->prev_;

            Div->recountPrev();
            Div->recountDepth();

            *lSub = *node_cur;
            lSub->setData({op_names[OP_MUL], NODE_OPERATOR});

            err = Differentiate(lSub);
            if (err) return err;

            lSub = Div->left_;
            lSub->setData({op_names[OP_SUB], NODE_OPERATOR});

            *rPow_l = *node_cur->right_;
            rPow_r->setData({{0, "2"}, NODE_NUMBER});

            break;
        }
        case OP_POW:
        {
            Node<CalcNodeData>* Mul  = new Node<CalcNodeData>;
            Node<CalcNodeData>* lPow = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSum = new Node<CalcNodeData>;

            Node<CalcNodeData>* lPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* lPow_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rlMul = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrMul = new Node<CalcNodeData>;

            Node<CalcNodeData>* rlMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rlrLn   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rlrLn_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrlDiv   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrlDiv_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrlDiv_r = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrMul_r  = new Node<CalcNodeData>;

            Mul ->setData({op_names[OP_MUL], NODE_OPERATOR});
            lPow->setData({op_names[OP_POW], NODE_OPERATOR});
            rSum->setData({op_names[OP_ADD], NODE_OPERATOR});

            rlMul->setData({op_names[OP_MUL], NODE_OPERATOR});
            rrMul->setData({op_names[OP_MUL], NODE_OPERATOR});

            rlrLn ->setData({op_names[OP_LN],  NODE_FUNCTION});
            rrlDiv->setData({op_names[OP_DIV], NODE_OPERATOR});

            Mul->left_  = lPow;
            Mul->right_ = rSum;

            lPow->left_  = lPow_l;
            lPow->right_ = lPow_r;

            rSum->left_  = rlMul;
            rSum->right_ = rrMul;

            rlMul->left_  = rlMul_l;
            rlMul->right_ = rlrLn;
            rlrLn->right_ = rlrLn_r;

            rrMul->left_  = rrlDiv;
            rrMul->right_ = rrMul_r;

            rrlDiv->left_  = rrlDiv_l;
            rrlDiv->right_ = rrlDiv_r;

            if (node_cur->prev_ != nullptr)
            {
                if (node_cur->prev_->left_ == node_cur) node_cur->prev_->left_  = Mul;
                else                                    node_cur->prev_->right_ = Mul;
            }

            Mul->prev_ = node_cur->prev_;

            Mul->recountPrev();
            Mul->recountDepth();

            *lPow_l = *node_cur->left_;
            *lPow_r = *node_cur->right_;

            *rlMul_l = *node_cur->right_;
            *rlrLn_r = *node_cur->left_;

            *rrlDiv_l = *node_cur->right_;
            *rrlDiv_r = *node_cur->left_;

            *rrMul_r  = *node_cur->left_;

            err = Differentiate(rlMul_l);
            if (err) return err;

            err = Differentiate(rrMul_r);
            if (err) return err;

            break;
        }
        case OP_ARCCOS:
        {
            Node<CalcNodeData>* Sub  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rDiv = new Node<CalcNodeData>;

            Node<CalcNodeData>* rDiv_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSqrt = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrrSub   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrSub_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrrPow  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrrrPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrrPow_r = new Node<CalcNodeData>;

            Sub ->setData({op_names[OP_SUB], NODE_OPERATOR});
            rDiv->setData({op_names[OP_DIV], NODE_OPERATOR});

            rrSqrt ->setData({op_names[OP_SQRT], NODE_FUNCTION});
            rrrSub ->setData({op_names[OP_SUB],  NODE_OPERATOR});
            rrrrPow->setData({op_names[OP_POW],  NODE_OPERATOR});

            Sub->right_ = rDiv;

            rDiv->left_  = rDiv_l;
            rDiv->right_ = rrSqrt;

            rrSqrt->right_ = rrrSub;
            rrrSub->left_  = rrrSub_l;
            rrrSub->right_ = rrrrPow;

            rrrrPow->left_  = rrrrPow_l;
            rrrrPow->right_ = rrrrPow_r;

            if (node_cur->prev_ != nullptr)
            {
                if (node_cur->prev_->left_ == node_cur) node_cur->prev_->left_  = Sub;
                else                                    node_cur->prev_->right_ = Sub;
            }

            Sub->prev_ = node_cur->prev_;

            Sub->recountPrev();
            Sub->recountDepth();

            *rDiv_l = *node_cur->right_;
            rrrSub_l->setData({{0, "1"}, NODE_NUMBER});
            
            *rrrrPow_l = *node_cur->right_;
            rrrrPow_r->setData({{0, "2"}, NODE_NUMBER});

            err = Differentiate(rDiv_l);
            if (err) return err;

            break;
        }
        case OP_ARCCOSH:
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSqrt = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrSub   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrlPow  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSub_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrlPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrlPow_r = new Node<CalcNodeData>;

            Div  ->setData({op_names[OP_DIV],  NODE_OPERATOR});
            rSqrt->setData({op_names[OP_SQRT], NODE_FUNCTION});

            rrSub ->setData({op_names[OP_SUB], NODE_OPERATOR});
            rrlPow->setData({op_names[OP_POW], NODE_OPERATOR});

            Div->left_  = Div_l;
            Div->right_ = rSqrt;

            rSqrt->right_ = rrSub;
            rrSub->left_  = rrlPow;
            rrSub->right_ = rrSub_r;

            rrlPow->left_  = rrlPow_l;
            rrlPow->right_ = rrlPow_r;

            if (node_cur->prev_ != nullptr)
            {
                if (node_cur->prev_->left_ == node_cur) node_cur->prev_->left_  = Div;
                else                                    node_cur->prev_->right_ = Div;
            }

            Div->prev_ = node_cur->prev_;

            Div->recountPrev();
            Div->recountDepth();

            *Div_l = *node_cur->right_;

            *rrlPow_l = *node_cur->right_;
            rrlPow_r->setData({{0, "2"}, NODE_NUMBER});

            rrSub_r->setData({{0, "1"}, NODE_NUMBER});

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_ARCCOT:
        {
            Node<CalcNodeData>* Sub  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rDiv = new Node<CalcNodeData>;

            Node<CalcNodeData>* rDiv_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSum  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrSum_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrPow  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrrPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrPow_r = new Node<CalcNodeData>;

            break;
        }
        case OP_ARCCOTH:
        {

            break;
        }
        case OP_ARCSIN:
        {

            break;
        }
        case OP_ARCSINH:
        {

            break;
        }
        case OP_ARCTAN:
        {

            break;
        }
        case OP_ARCTANH:
        {

            break;
        }
        case OP_CBRT:
        {

            break;
        }
        case OP_COS:
        {

            break;
        }
        case OP_COSH:
        {

            break;
        }
        case OP_COT:
        {

            break;
        }
        case OP_COTH:
        {

            break;
        }
        case OP_EXP:
        {

            break;
        }
        case OP_LG:
        {

            break;
        }
        case OP_LN:
        {

            break;
        }
        case OP_SIN:
        {

            break;
        }
        case OP_SINH:
        {

            break;
        }
        case OP_SQRT:
        {

            break;
        }
        case OP_TAN:
        {

            break;
        }
        case OP_TANH:
        {

            break;
        }
        default: assert(0);
        }
        break;

    case NODE_VARIABLE:
    {

        break;
    }

    case NODE_NUMBER:
    {

        break;
    }

    default: assert(0);
    }

    node_cur->~Node();
    delete node_cur;

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
