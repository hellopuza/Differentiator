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
    constants_    ((char*)"variables"),
    path2badnode_ ((char*)"path2badnode_"),
    state_        (DIFF_OK)
{
    constants_.Push({ PI, "pi" });
    constants_.Push({ E,  "e" });
}

//------------------------------------------------------------------------------

Differentiator::Differentiator (char* filename) :
    filename_     (filename),
    tree_         (GetTrueFileName(filename)),
    constants_    ((char*)"variables"),
    path2badnode_ ((char*)"path2badnode_"),
    state_        (DIFF_OK)
{
    constants_.Push({ PI, "pi" });
    constants_.Push({ E,  "e" });
}

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
                Differentiate(tree_.root_);
                Optimize(tree_);

                printExprGraph(tree_);
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

        Differentiate(tree_.root_);
        Optimize(tree_);

        printExprGraph(tree_);
        Write();
    }
    
    return DIFF_OK;
}

//------------------------------------------------------------------------------

#define PREV_CONNECT(old_node, new_node)                \
        {                                               \
            if (old_node->prev_ != nullptr)             \
            {                                           \
                if (old_node->prev_->left_ == old_node) \
                    old_node->prev_->left_  = new_node; \
                else                                    \
                    old_node->prev_->right_ = new_node; \
            }                                           \
            else tree_.root_ = new_node;                \
                                                        \
            new_node->prev_ = old_node->prev_;          \
                                                        \
            new_node->recountPrev();                    \
            new_node->recountDepth();                   \
        } //

//------------------------------------------------------------------------------

int Differentiator::Differentiate (Node<CalcNodeData>* node_cur)
{
    int err = DIFF_OK;

    switch (node_cur->getData().node_type)
    {
    case NODE_FUNCTION:
    case NODE_OPERATOR:

        switch (node_cur->getData().op_code)
        {
        case OP_ADD:       // u' + v'
        case OP_SUB:       // u' - v'
        {
            Node<CalcNodeData>* Sum   = new Node<CalcNodeData>;
            
            Node<CalcNodeData>* Sum_l = nullptr;
            if (node_cur->left_ != nullptr)
                Sum_l = new Node<CalcNodeData>;

            Node<CalcNodeData>* Sum_r = new Node<CalcNodeData>;

            Sum->setData({ POISON<NUM_TYPE>, op_names[node_cur->getData().op_code].word, op_names[node_cur->getData().op_code].code, NODE_OPERATOR });

            Sum->left_  = Sum_l;
            Sum->right_ = Sum_r;

            PREV_CONNECT(node_cur, Sum);
            
            if (node_cur->left_ != nullptr)
                *Sum_l = *node_cur->left_;

            *Sum_r = *node_cur->right_;

            if (node_cur->left_ != nullptr)
            {
                err = Differentiate(Sum_l);
                if (err) return err;
            }

            err = Differentiate(Sum_r);
            if (err) return err;

            break;
        }
        case OP_MUL:       // u'*v + u*v'
        {
            Node<CalcNodeData>* Sum  = new Node<CalcNodeData>;
            Node<CalcNodeData>* lMul = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul = new Node<CalcNodeData>;

            Node<CalcNodeData>* lMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* lMul_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul_r = new Node<CalcNodeData>;

            Sum ->setData({ POISON<NUM_TYPE>, op_names[OP_ADD].word, op_names[OP_ADD].code, NODE_OPERATOR });
            lMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });
            rMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });

            Sum->left_  = lMul;
            Sum->right_ = rMul;

            lMul->left_  = lMul_l;
            lMul->right_ = lMul_r;
            
            rMul->left_  = rMul_l;
            rMul->right_ = rMul_r;

            PREV_CONNECT(node_cur, Sum);

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
        case OP_DIV:       // (u'*v - u*v')/v^2
        {
            Node<CalcNodeData>* Div  = new Node<CalcNodeData>;
            Node<CalcNodeData>* lSub = new Node<CalcNodeData>;
            Node<CalcNodeData>* rPow = new Node<CalcNodeData>;

            Node<CalcNodeData>* rPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rPow_r = new Node<CalcNodeData>;

            Div ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });
            rPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            Div->left_  = lSub;
            Div->right_ = rPow;

            rPow->left_  = rPow_l;
            rPow->right_ = rPow_r;

            PREV_CONNECT(node_cur, Div);

            *lSub = *node_cur;
            lSub->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });

            err = Differentiate(lSub);
            if (err) return err;

            lSub = Div->left_;
            lSub->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });

            *rPow_l = *node_cur->right_;
            rPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            break;
        }
        case OP_POW:       // u^v*(v'*ln(u) + v/u*u')
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

            Mul ->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });
            lPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });
            rSum->setData({ POISON<NUM_TYPE>, op_names[OP_ADD].word, op_names[OP_ADD].code, NODE_OPERATOR });

            rlMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });
            rrMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });

            rlrLn ->setData({ POISON<NUM_TYPE>, op_names[OP_LN].word,  op_names[OP_LN].code,  NODE_FUNCTION });
            rrlDiv->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });

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

            PREV_CONNECT(node_cur, Mul);

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
        case OP_ARCCOS:    // -u'/sqrt(1 - u^2)
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

            Sub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            rDiv->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });

            rrSqrt ->setData({ POISON<NUM_TYPE>, op_names[OP_SQRT].word, op_names[OP_SQRT].code, NODE_FUNCTION });
            rrrSub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word,  op_names[OP_SUB].code,  NODE_OPERATOR });
            rrrrPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word,  op_names[OP_POW].code,  NODE_OPERATOR });

            Sub->right_ = rDiv;

            rDiv->left_  = rDiv_l;
            rDiv->right_ = rrSqrt;

            rrSqrt->right_ = rrrSub;
            rrrSub->left_  = rrrSub_l;
            rrrSub->right_ = rrrrPow;

            rrrrPow->left_  = rrrrPow_l;
            rrrrPow->right_ = rrrrPow_r;

            PREV_CONNECT(node_cur, Sub);

            *rDiv_l = *node_cur->right_;
            rrrSub_l->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });
            
            *rrrrPow_l = *node_cur->right_;
            rrrrPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(rDiv_l);
            if (err) return err;

            break;
        }
        case OP_ARCCOSH:   // u'/sqrt(u^2 - 1)
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSqrt = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrSub   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrlPow  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSub_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrlPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrlPow_r = new Node<CalcNodeData>;

            Div  ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word,  op_names[OP_DIV].code,  NODE_OPERATOR });
            rSqrt->setData({ POISON<NUM_TYPE>, op_names[OP_SQRT].word, op_names[OP_SQRT].code, NODE_FUNCTION });

            rrSub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            rrlPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            Div->left_  = Div_l;
            Div->right_ = rSqrt;

            rSqrt->right_ = rrSub;
            rrSub->left_  = rrlPow;
            rrSub->right_ = rrSub_r;

            rrlPow->left_  = rrlPow_l;
            rrlPow->right_ = rrlPow_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l = *node_cur->right_;

            *rrlPow_l = *node_cur->right_;
            rrlPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            rrSub_r->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_ARCCOT:    // -u'/(1 + u^2)
        {
            Node<CalcNodeData>* Sub  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rDiv = new Node<CalcNodeData>;

            Node<CalcNodeData>* rDiv_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSum  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrSum_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrPow  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrrPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrPow_r = new Node<CalcNodeData>;

            Sub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            rDiv->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });

            rrSum ->setData({ POISON<NUM_TYPE>, op_names[OP_ADD].word, op_names[OP_ADD].code, NODE_OPERATOR });
            rrrPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            Sub->right_ = rDiv;

            rDiv->left_  = rDiv_l;
            rDiv->right_ = rrSum;

            rrSum->left_  = rrSum_l;
            rrSum->right_ = rrrPow;

            rrrPow->left_  = rrrPow_l;
            rrrPow->right_ = rrrPow_r;

            PREV_CONNECT(node_cur, Sub);

            *rDiv_l = *node_cur->right_;
            rrSum_l->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });

            *rrrPow_l = *node_cur->right_;
            rrrPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(rDiv_l);
            if (err) return err;

            break;
        }
        case OP_ARCCOTH:   // u'/(1 - u^2)
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSub  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rSub_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrPow  = new Node<CalcNodeData>;
            
            Node<CalcNodeData>* rrPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrPow_r = new Node<CalcNodeData>;

            Div  ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });
            rSub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            rrPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            Div->left_  = Div_l;
            Div->right_ = rSub;

            rSub->left_  = rSub_l;
            rSub->right_ = rrPow;

            rrPow->left_  = rrPow_l;
            rrPow->right_ = rrPow_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l = *node_cur->right_;
            rSub_l->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });

            *rrPow_l = *node_cur->right_;
            rrPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_ARCSIN:    // u'/sqrt(1 - u^2)
        case OP_ARCSINH:   // u'/sqrt(1 + u^2)
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSqrt = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrSum   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSum_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrPow  = new Node<CalcNodeData>;
            
            Node<CalcNodeData>* rrrPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrrPow_r = new Node<CalcNodeData>;

            Div  ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word,  op_names[OP_DIV].code,  NODE_OPERATOR });
            rSqrt->setData({ POISON<NUM_TYPE>, op_names[OP_SQRT].word, op_names[OP_SQRT].code, NODE_FUNCTION });

            if ((node_cur->getData().op_code) == OP_ARCSIN)
                rrSum->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            else
                rrSum->setData({ POISON<NUM_TYPE>, op_names[OP_ADD].word, op_names[OP_ADD].code, NODE_OPERATOR });

            rrrPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            Div->left_  = Div_l;
            Div->right_ = rSqrt;

            rSqrt->right_ = rrSum;
            rrSum->left_  = rrSum_l;
            rrSum->right_ = rrrPow;

            rrrPow->left_  = rrrPow_l;
            rrrPow->right_ = rrrPow_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l = *node_cur->right_;
            rrSum_l->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });

            *rrrPow_l = *node_cur->right_;
            rrrPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_ARCTAN:    // u'/(1 + u^2)
        case OP_ARCTANH:   // u'/(1 - u^2)
        {
            Node<CalcNodeData>* Div = new Node<CalcNodeData>;

            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSum  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rSum_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrPow  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrPow_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrPow_r = new Node<CalcNodeData>;

            Div->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });

            if ((node_cur->getData().op_code) == OP_ARCTAN)
                rSum->setData({ POISON<NUM_TYPE>, op_names[OP_ADD].word, op_names[OP_ADD].code, NODE_OPERATOR });
            else
                rSum->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });

            rrPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            Div->left_  = Div_l;
            Div->right_ = rSum;

            rSum->left_  = rSum_l;
            rSum->right_ = rrPow;

            rrPow->left_  = rrPow_l;
            rrPow->right_ = rrPow_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l = *node_cur->right_;
            rSum_l->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });

            *rrPow_l = *node_cur->right_;
            rrPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_COS:       // -u'*sin(u)
        {
            Node<CalcNodeData>* Sub  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul = new Node<CalcNodeData>;

            Node<CalcNodeData>* rMul_l  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSin   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSin_r = new Node<CalcNodeData>;
            
            Sub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            rMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });

            rrSin->setData({ POISON<NUM_TYPE>, op_names[OP_SIN].word, op_names[OP_SIN].code, NODE_FUNCTION });

            Sub->right_ = rMul;

            rMul->left_  = rMul_l;
            rMul->right_ = rrSin;

            rrSin->right_ = rrSin_r;

            PREV_CONNECT(node_cur, Sub);

            *rMul_l  = *node_cur->right_;
            *rrSin_r = *node_cur->right_;

            err = Differentiate(rMul_l);
            if (err) return err;

            break;
        }
        case OP_COSH:      // u'*sinh(u)
        {
            Node<CalcNodeData>* Mul = new Node<CalcNodeData>;

            Node<CalcNodeData>* Mul_l   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSinh   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rSinh_r = new Node<CalcNodeData>;
            
            Mul  ->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word,  op_names[OP_MUL].code,  NODE_OPERATOR });
            rSinh->setData({ POISON<NUM_TYPE>, op_names[OP_SINH].word, op_names[OP_SINH].code, NODE_FUNCTION });

            Mul->left_  = Mul_l;
            Mul->right_ = rSinh;

            rSinh->right_ = rSinh_r;

            PREV_CONNECT(node_cur, Mul);

            *Mul_l   = *node_cur->right_;
            *rSinh_r = *node_cur->right_;

            err = Differentiate(Mul_l);
            if (err) return err;

            break;
        }
        case OP_COT:       // -u'/(sin(u)^2)
        case OP_COTH:      // -u'/(sinh(u)^2)
        {
            Node<CalcNodeData>* Sub  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rDiv = new Node<CalcNodeData>;

            Node<CalcNodeData>* rDiv_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrPow  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrlSin  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrPow_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rrlSin_r = new Node<CalcNodeData>;
            
            Sub ->setData({ POISON<NUM_TYPE>, op_names[OP_SUB].word, op_names[OP_SUB].code, NODE_OPERATOR });
            rDiv->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });

            rrPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            if ((node_cur->getData().op_code) == OP_COT)
                rrlSin->setData({ POISON<NUM_TYPE>, op_names[OP_SIN].word, op_names[OP_SIN].code, NODE_FUNCTION });
            else
                rrlSin->setData({ POISON<NUM_TYPE>, op_names[OP_SINH].word, op_names[OP_SINH].code, NODE_FUNCTION });

            Sub->right_ = rDiv;

            rDiv->left_  = rDiv_l;
            rDiv->right_ = rrPow;

            rrPow->left_  = rrlSin;
            rrPow->right_ = rrPow_r;

            rrlSin->right_ = rrlSin_r;

            PREV_CONNECT(node_cur, Sub);

            *rDiv_l   = *node_cur->right_;
            *rrlSin_r = *node_cur->right_;

            rrPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(rDiv_l);
            if (err) return err;

            break;
        }
        case OP_EXP:       // u'*exp(u)
        {
            Node<CalcNodeData>* Mul   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Mul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rExp  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rExp_r = new Node<CalcNodeData>;
            
            Mul ->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });
            rExp->setData({ POISON<NUM_TYPE>, op_names[OP_EXP].word, op_names[OP_EXP].code, NODE_FUNCTION });

            Mul->left_  = Mul_l;
            Mul->right_ = rExp;

            rExp->right_ = rExp_r;

            PREV_CONNECT(node_cur, Mul);

            *Mul_l  = *node_cur->right_;
            *rExp_r = *node_cur->right_;

            err = Differentiate(Mul_l);
            if (err) return err;

            break;
        }
        case OP_LG:        // u'/(u*ln(10))
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrLn   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrLn_r = new Node<CalcNodeData>;
            
            Div ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });
            rMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });
            rrLn->setData({ POISON<NUM_TYPE>, op_names[OP_LN].word,  op_names[OP_LN].code,  NODE_FUNCTION });

            Div->left_  = Div_l;
            Div->right_ = rMul;

            rMul->left_  = rMul_l;
            rMul->right_ = rrLn;

            rrLn->right_ = rrLn_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l  = *node_cur->right_;
            *rMul_l = *node_cur->right_;

            rrLn_r->setData({ NUM_TYPE{10, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_LN:        // u'/u
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_r  = new Node<CalcNodeData>;
            
            Div->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });

            Div->left_  = Div_l;
            Div->right_ = Div_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l = *node_cur->right_;
            *Div_r = *node_cur->right_;

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_SIN:       // u'*cos(u)
        case OP_SINH:      // u'*cosh(u)
        {
            Node<CalcNodeData>* Mul = new Node<CalcNodeData>;

            Node<CalcNodeData>* Mul_l  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rCos   = new Node<CalcNodeData>;
            Node<CalcNodeData>* rCos_r = new Node<CalcNodeData>;
            
            Mul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });

            if ((node_cur->getData().op_code) == OP_SIN)
                rCos->setData({ POISON<NUM_TYPE>, op_names[OP_COS].word, op_names[OP_COS].code, NODE_FUNCTION });
            else
                rCos->setData({ POISON<NUM_TYPE>, op_names[OP_COSH].word, op_names[OP_COSH].code, NODE_FUNCTION });

            Mul->left_  = Mul_l;
            Mul->right_ = rCos;

            rCos->right_ = rCos_r;

            PREV_CONNECT(node_cur, Mul);

            *Mul_l  = *node_cur->right_;
            *rCos_r = *node_cur->right_;

            err = Differentiate(Mul_l);
            if (err) return err;

            break;
        }
        case OP_SQRT:      // u'/(2*sqrt(u))
        {
            Node<CalcNodeData>* Div   = new Node<CalcNodeData>;
            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rMul  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rMul_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rrSqrt  = new Node<CalcNodeData>;
            
            Node<CalcNodeData>* rrSqrt_r = new Node<CalcNodeData>;

            Div ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });
            rMul->setData({ POISON<NUM_TYPE>, op_names[OP_MUL].word, op_names[OP_MUL].code, NODE_OPERATOR });

            rrSqrt->setData({ POISON<NUM_TYPE>, op_names[OP_SQRT].word, op_names[OP_SQRT].code, NODE_FUNCTION });

            Div->left_  = Div_l;
            Div->right_ = rMul;

            rMul->left_  = rMul_l;
            rMul->right_ = rrSqrt;

            rrSqrt->right_ = rrSqrt_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l = *node_cur->right_;
            rMul_l->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            *rrSqrt_r = *node_cur->right_;

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        case OP_TAN:       // u'/(cos(u)^2)
        case OP_TANH:      // u'/(cosh(u)^2)
        {
            Node<CalcNodeData>* Div = new Node<CalcNodeData>;

            Node<CalcNodeData>* Div_l = new Node<CalcNodeData>;
            Node<CalcNodeData>* rPow  = new Node<CalcNodeData>;

            Node<CalcNodeData>* rlCos  = new Node<CalcNodeData>;
            Node<CalcNodeData>* rPow_r = new Node<CalcNodeData>;

            Node<CalcNodeData>* rlCos_r = new Node<CalcNodeData>;
            
            Div ->setData({ POISON<NUM_TYPE>, op_names[OP_DIV].word, op_names[OP_DIV].code, NODE_OPERATOR });
            rPow->setData({ POISON<NUM_TYPE>, op_names[OP_POW].word, op_names[OP_POW].code, NODE_OPERATOR });

            if ((node_cur->getData().op_code) == OP_TAN)
                rlCos->setData({ POISON<NUM_TYPE>, op_names[OP_COS].word, op_names[OP_COS].code, NODE_FUNCTION });
            else
                rlCos->setData({ POISON<NUM_TYPE>, op_names[OP_COSH].word, op_names[OP_COSH].code, NODE_FUNCTION });

            Div->left_  = Div_l;
            Div->right_ = rPow;

            rPow->left_  = rlCos;
            rPow->right_ = rPow_r;

            rlCos->right_ = rlCos_r;

            PREV_CONNECT(node_cur, Div);

            *Div_l   = *node_cur->right_;
            *rlCos_r = *node_cur->right_;

            rPow_r->setData({ NUM_TYPE{2, 0}, nullptr, 0, NODE_NUMBER });

            err = Differentiate(Div_l);
            if (err) return err;

            break;
        }
        default: assert(0);
        }
        break;

    case NODE_VARIABLE:
    {
        Node<CalcNodeData>* Num = new Node<CalcNodeData>;

        if (strcmp(node_cur->getData().word, diff_var_.name) == 0)
            Num->setData({ NUM_TYPE{1, 0}, nullptr, 0, NODE_NUMBER });
        else
            Num->setData({ NUM_TYPE{0, 0}, nullptr, 0, NODE_NUMBER });

        PREV_CONNECT(node_cur, Num);

        break;
    }
    case NODE_NUMBER:
    {
        Node<CalcNodeData>* Num = new Node<CalcNodeData>;

        Num->setData({ NUM_TYPE{0, 0}, nullptr, 0, NODE_NUMBER });

        PREV_CONNECT(node_cur, Num);

        break;
    }

    default: assert(0);
    }

    delete node_cur;

    return DIFF_OK;
}

//------------------------------------------------------------------------------

void Differentiator::Write ()
{
    char* str = new char[MAX_STR_LEN] {};
    Expression expr = {str, str};
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

    delete [] str;
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
