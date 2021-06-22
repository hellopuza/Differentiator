/*------------------------------------------------------------------------------
    * File:        Calculator.cpp                                              *
    * Description: Functions for calculating math expressions.                 *
    * Created:     15 may 2021                                                 *
    * Author:      Artem Puzankov                                              *
    * Email:       puzankov.ao@phystech.edu                                    *
    * GitHub:      https://github.com/hellopuza                                *
    * Copyright © 2021 Artem Puzankov. All rights reserved.                    *
    *///------------------------------------------------------------------------

#include "Calculator.h"

//------------------------------------------------------------------------------

Calculator::Calculator () :
    filename_     (nullptr),
    trees_        ((char*)"trees of expression"),
    variables_    ((char*)"variables"),
    state_        (CALC_OK)
{
    Tree<CalcNodeData> tree((char*)"expression");
    trees_.Push(tree);

    ADD_VAR(variables_);
}

//------------------------------------------------------------------------------

Calculator::Calculator (char* filename) :
    filename_     (filename),
    trees_        ((char*)"trees of expression"),
    variables_    ((char*)"variables"),
    state_        (CALC_OK)
{
    Tree<CalcNodeData> tree(GetTrueFileName(filename));
    trees_.Push(tree);

    ADD_VAR(variables_);
}

//------------------------------------------------------------------------------

Calculator::~Calculator ()
{
    CALC_ASSERTOK((this == nullptr),           CALC_NULL_INPUT_CALCULATOR_PTR);
    CALC_ASSERTOK((state_ == CALC_DESTRUCTED), CALC_DESTRUCTED               );

    filename_ = nullptr;

    state_ = CALC_DESTRUCTED;
}

//------------------------------------------------------------------------------

int Calculator::Run ()
{
    CALC_ASSERTOK((this == nullptr), CALC_NULL_INPUT_CALCULATOR_PTR);

    if (filename_ == nullptr)
    {
        bool running = true;
        while (running)
        {
            printf("\nEnter expression: ");

            char* expr = ScanExpr();
            Expression expression = { expr, expr };

            int err = Expr2Tree(expression, trees_[0]);
            delete [] expr;
            if (!err)
            {
                trees_[0].Dump();

                Calculate(trees_[0].root_);
                Write();
            }

            trees_.Clean();
            variables_.Clean();

            ADD_VAR(variables_);

            printf("Continue [Y/n]? ");
            running = scanAns();
        }
    }
    else
    {
        Text text(filename_);
        char* expr = text.text_;
        Expression expression = { expr, expr };

        int err = Expr2Tree(expression, trees_[0]);
        delete [] expr;
        if (err) return err;

        trees_[0].Dump();

        Calculate(trees_[0].root_);
        Write();
    }
    
    return CALC_OK;
}

//------------------------------------------------------------------------------

int Calculator::Calculate (Node<CalcNodeData>* node_cur)
{
    assert(node_cur != nullptr);

    int err = 0;
    int index = -1;

    NUM_TYPE number    = 0;
    NUM_TYPE right_num = 0;
    NUM_TYPE left_num  = 0;

    char* num_str = new char [MAX_STR_LEN] {};

    switch (node_cur->getData().node_type)
    {
    case NODE_FUNCTION:
    {
        assert((node_cur->right_ != nullptr) && (node_cur->left_ == nullptr));
        Calculate(node_cur->right_);
        assert(node_cur->right_->getData().node_type == NODE_NUMBER);

        number = Str2Num(node_cur->right_->getData().op.word);

        using namespace std;
        #define ONE static_cast<NUM_TYPE>(1)
        #define TWO static_cast<NUM_TYPE>(2)

        switch (node_cur->getData().op.code)
        {
        case OP_ARCCOS:     number = acos(number);          break;
        case OP_ARCCOSH:    number = acosh(number);         break;
        case OP_ARCCOT:     number = PI/TWO - atan(number); break;
        case OP_ARCCOTH:    number = atanh(ONE / number);   break;
        case OP_ARCSIN:     number = asin(number);          break;
        case OP_ARCSINH:    number = asinh(number);         break;
        case OP_ARCTAN:     number = atan(number);          break;
        case OP_ARCTANH:    number = atanh(number);         break;
        case OP_COS:        number = cos(number);           break;
        case OP_COSH:       number = cosh(number);          break;
        case OP_COT:        number = ONE / tan(number);     break;
        case OP_COTH:       number = ONE / tanh(number);    break;
        case OP_EXP:        number = exp(number);           break;
        case OP_LG:         number = log10(number);         break;
        case OP_LN:         number = log(number);           break;
        case OP_SIN:        number = sin(number);           break;
        case OP_SINH:       number = sinh(number);          break;
        case OP_SQRT:       number = sqrt(number);          break;
        case OP_TAN:        number = tan(number);           break;
        case OP_TANH:       number = tanh(number);          break;
        default: assert(0);
        }

        #undef ONE
        #undef TWO

        node_cur->right_->~Node();
        delete node_cur->right_;
        node_cur->right_ = nullptr;

        sprintf(num_str, NUM_PRINT_FORMAT, number);

        node_cur->setData({ {0, num_str}, NODE_NUMBER });
        break;
    }
    case NODE_OPERATOR:
    {
        if (node_cur->left_ != nullptr)
        {
            Calculate(node_cur->left_);
            assert(node_cur->left_->getData().node_type == NODE_NUMBER);

            left_num = Str2Num(node_cur->left_->getData().op.word);
        }
        else left_num = 0;

        Calculate(node_cur->right_);
        assert(node_cur->right_->getData().node_type == NODE_NUMBER);

        right_num = Str2Num(node_cur->right_->getData().op.word);

        switch (node_cur->getData().op.code)
        {
        case OP_MUL:  number = left_num * right_num;     break;
        case OP_ADD:  number = left_num + right_num;     break;
        case OP_DIV:  number = left_num / right_num;     break;
        case OP_SUB:  number = left_num - right_num;     break;
        case OP_POW:  number = pow(left_num, right_num); break;
        default: assert(0);
        }

        node_cur->right_->~Node();
        delete node_cur->right_;
        node_cur->right_ = nullptr;

        if (node_cur->left_ != nullptr)
        {
            node_cur->left_->~Node();
            delete node_cur->left_;
            node_cur->left_ = nullptr;
        }

        sprintf(num_str, NUM_PRINT_FORMAT, number);

        node_cur->setData({ {0, num_str}, NODE_NUMBER });
        break;
    }
    case NODE_VARIABLE:
    {
        assert((node_cur->right_ == nullptr) && (node_cur->left_ == nullptr));

        index = -1;
        for (int i = 0; i < variables_.getSize(); ++i)
            if (strcmp(variables_[i].name, node_cur->getData().op.word) == 0)
            {
                index = i;
                break;
            }
        
        if (index == -1)
        {
            variables_.Push({ 0, node_cur->getData().op.word });
            size_t size = variables_.getSize();

            number = scanVar(*this, node_cur->getData().op.word);
            variables_[size - 1] = { number, node_cur->getData().op.word };
        }
        else number = variables_[index].value;

        sprintf(num_str, NUM_PRINT_FORMAT, number);

        node_cur->setData({ {0, num_str}, NODE_NUMBER });
        break;
    }
    case NODE_NUMBER:
        break;

    default: assert(0);
    }

    return CALC_OK;
}

//------------------------------------------------------------------------------

void Calculator::Write ()
{
    if (filename_ == nullptr)
    {
        printf("result: %s\n", trees_[0].root_->getData().op.word);
    }
    else
    {
        FILE* output = fopen(filename_, "w");
        assert(output != nullptr);

        fprintf(output, "%s", trees_[0].root_->getData().op.word);
        fclose(output);
    }
}

//------------------------------------------------------------------------------

void CalcPrintError (const char* logname, const char* file, int line, const char* function, int err, bool console_err)
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
    fprintf(log, "%s\n", calc_errstr[err + 1]);

    if (console_err)
        printf(  "ERROR: file %s  line %d  function %s\n\n", file, line, function);

    printf (     "ERROR: %s\n\n", calc_errstr[err + 1]);
}

//------------------------------------------------------------------------------

void TypePrint (FILE* fp, const CalcNodeData& node_data)
{
    switch (node_data.node_type)
    {
        case NODE_FUNCTION: fprintf(fp, "func: "); break;
        case NODE_OPERATOR: fprintf(fp, "oper: "); break;
        case NODE_VARIABLE: fprintf(fp, "var: " ); break;
        case NODE_NUMBER:   fprintf(fp, "num: " ); break;
        default:            fprintf(fp, "err: " ); break;
    }

    fprintf(fp, "%s", node_data.op.word);
}

//------------------------------------------------------------------------------

void TypePrint (FILE* fp, const Variable& var)
{
    fprintf(fp, "%s: " NUM_PRINT_FORMAT, var.name, var.value);
}

//------------------------------------------------------------------------------

bool isPOISON (CalcNodeData value)
{
    return ( (value.op.word   == 0) &&
             (value.op.code   == 0) &&
             (value.node_type == 0)   );
}

//------------------------------------------------------------------------------

bool isPOISON (Variable var)
{
    return ( (var.value == static_cast<NUM_TYPE>(0)) &&
             (var.name  == nullptr) );
}

//------------------------------------------------------------------------------

bool scanAns ()
{
    char ans[MAX_STR_LEN] = "";

    char* err = fgets(ans, MAX_STR_LEN - 2, stdin);
    ans[0] = toupper(ans[0]);

    while ((ans[0] != 'Y') && (ans[0] != 'N') || (ans[1] != '\n') || !err)
    {
        printf("Try again [Y/n]? ");
        err = fgets(ans, MAX_STR_LEN - 2, stdin);
        ans[0] = toupper(ans[0]);
    }

    return ((ans[0] == 'Y') ? 1 : 0);
}

//------------------------------------------------------------------------------

NUM_TYPE scanVar (Calculator& calc, char* varname)
{
    printf("Enter value of variable %s: ", varname);

    char* expr = ScanExpr();
    Expression expression = { expr, expr };

    Tree<CalcNodeData> vartree(varname);
    while (Expr2Tree(expression, vartree))
    {
        delete [] expr;
        printf("Try again: ");
        expr = ScanExpr();
        Expression expression = { expr, expr };
    }
    delete [] expr;

    calc.trees_.Push(vartree);
    calc.Calculate(calc.trees_[calc.trees_.getSize() - 1].root_);

    return Str2Num(calc.trees_[calc.trees_.getSize() - 1].root_->getData().op.word);
}

//------------------------------------------------------------------------------

NUM_TYPE Str2Num (char* str)
{
    char* endstr = nullptr;
    NUM_TYPE number = 0;
    double num = 0;
    char sign = 0;
    num = strtod(str, &endstr);

    if ((str == endstr) && (*str == 'i'))
	return I;

    if (*endstr == 'i')
        return num * I;
    else
        number += num;
    
    if (*endstr == '+')
    {
        sign = 1;
        ++endstr;
    }
    else if (*endstr == '-')
    {
        sign = -1;
        ++endstr;
    }
    else if (*endstr == '\0')
    {
        return number;
    }

    str = endstr;
    endstr = nullptr;
    num = strtod(str, &endstr);

    if ((str == endstr) && (*str == 'i'))
	num = 1;

    number += num * sign * I;

    return number;
}

//------------------------------------------------------------------------------

char* ScanExpr ()
{
    char* expr = new char [MAX_STR_LEN] {};
    char err   = 0;

    do err = (not fgets(expr, MAX_STR_LEN - 2, stdin)) || (*expr == '\n');
    while (err && printf("Try again: "));
    
    return expr;
}

//------------------------------------------------------------------------------

int Tree2Expr (const Tree<CalcNodeData>& tree, Expression& expr)
{
    assert(expr.str != nullptr);
    expr.symb_cur = expr.str;
    int err = Node2Str(tree.root_, &expr.symb_cur);
    CALC_ASSERTOK(err, err);

    expr.symb_cur = expr.str;

    return err;
}

//------------------------------------------------------------------------------

int Node2Str (Node<CalcNodeData>* node_cur, char** str)
{
    assert(node_cur != nullptr);
    assert(*str     != nullptr);

    int err = 0;

    switch (node_cur->getData().node_type)
    {
    case NODE_FUNCTION:

        if ((node_cur->right_ == nullptr) || (node_cur->left_ != nullptr))
            return CALC_TREE_FUNC_WRONG_ARGUMENT;

        sprintf(*str, "%s", node_cur->getData().op.word);
        *str += strlen(node_cur->getData().op.word);

        sprintf(*str, "(");
        *str += 1;

        err = Node2Str(node_cur->right_, str);
        if (err) return err;

        sprintf(*str, ")");
        *str += 1;
        break;

    case NODE_OPERATOR:

        if ((node_cur->right_ == nullptr) ||
            (node_cur->left_  == nullptr) && (node_cur->getData().op.code != OP_SUB))
            return CALC_TREE_OPER_WRONG_ARGUMENTS;

        if (needBrackets(node_cur, node_cur->left_))
        {
            sprintf(*str, "(");
            *str += 1;

            if (node_cur->left_ != nullptr)
            {
                err = Node2Str(node_cur->left_, str);
                if (err) return err;
            }

            sprintf(*str, ")");
            *str += 1;
        }
        else
        if (node_cur->left_ != nullptr)
        {
            err = Node2Str(node_cur->left_, str);
            if (err) return err;
        }

        sprintf(*str, "%s", node_cur->getData().op.word);
        *str += 1;

        if (needBrackets(node_cur, node_cur->right_))
        {
            sprintf(*str, "(");
            *str += 1;

            err = Node2Str(node_cur->right_, str);
            if (err) return err;

            sprintf(*str, ")");
            *str += 1;
        }
        else
        {
            err = Node2Str(node_cur->right_, str);
            if (err) return err;
        }
        break;
    
    case NODE_VARIABLE:

        if ((node_cur->right_ != nullptr) || (node_cur->left_ != nullptr))
            return CALC_TREE_VAR_WRONG_ARGUMENT;
    
    case NODE_NUMBER:

        if ((node_cur->right_ != nullptr) || (node_cur->left_ != nullptr))
            return CALC_TREE_NUM_WRONG_ARGUMENT;

        sprintf(*str, "%s", node_cur->getData().op.word);
        *str += strlen(node_cur->getData().op.word);

        break;

    default: assert(0);
    }

    return CALC_OK;
}

//------------------------------------------------------------------------------

int Expr2Tree (Expression& expr, Tree<CalcNodeData>& tree)
{
    assert(expr.str != nullptr);

    del_spaces(expr.str);
    str_tolower(expr.str);

    tree.root_ = pass_Plus_Minus(expr);
    if (tree.root_ == nullptr) return -1;

    tree.root_->recountPrev();
    tree.root_->recountDepth();

    return 0;
}

//------------------------------------------------------------------------------

Node<CalcNodeData>* pass_Plus_Minus (Expression& expr)
{
    Node<CalcNodeData>* node_cur = nullptr;

    if (*expr.symb_cur == '-')
    {   
        ++expr.symb_cur;

        Node<CalcNodeData>* right = pass_Mul_Div(expr);

        node_cur = new Node<CalcNodeData>;
        node_cur->setData({op_names[OP_SUB], NODE_OPERATOR});
        node_cur->right_ = right;
    }
    else node_cur = pass_Mul_Div(expr);
    
    while ( (*expr.symb_cur == '+') ||
            (*expr.symb_cur == '-')   )
    {
        char* symb_cur = expr.symb_cur;
        ++expr.symb_cur;

        Node<CalcNodeData>* left  = node_cur;
        Node<CalcNodeData>* right = pass_Mul_Div(expr);

        node_cur = new Node<CalcNodeData>;
        node_cur->setData({op_names[(*symb_cur == '-') ? OP_SUB : OP_ADD], NODE_OPERATOR});
        node_cur->right_ = right;
        node_cur->left_  = left;
    }

    CHECK_SYNTAX(( (*expr.symb_cur != '+') &&
                   (*expr.symb_cur != '-') &&
                   (*expr.symb_cur != '*') &&
                   (*expr.symb_cur != '/') &&
                   (*expr.symb_cur != '^') &&
                   (*expr.symb_cur != '(') &&
                   (*expr.symb_cur != ')') &&
                   (*expr.symb_cur != '\0')   ), CALC_SYNTAX_ERROR, expr, 1);

    return node_cur;
}

//------------------------------------------------------------------------------

Node<CalcNodeData>* pass_Mul_Div (Expression& expr)
{
    Node<CalcNodeData>* node_cur = pass_Power(expr);

    while ( (*expr.symb_cur == '*') ||
            (*expr.symb_cur == '/')   )
    {
        char* symb_cur = expr.symb_cur;
        ++expr.symb_cur;

        Node<CalcNodeData>* left  = node_cur;
        Node<CalcNodeData>* right = pass_Power(expr);

        node_cur = new Node<CalcNodeData>;
        node_cur->setData({op_names[(*symb_cur == '*') ? OP_MUL : OP_DIV], NODE_OPERATOR});
        node_cur->right_ = right;
        node_cur->left_  = left;
    }

    return node_cur;
}

//------------------------------------------------------------------------------

Node<CalcNodeData>* pass_Power (Expression& expr)
{
    Node<CalcNodeData>* node_cur = pass_Brackets(expr);

    while (*expr.symb_cur == '^')
    {
        ++expr.symb_cur;

        Node<CalcNodeData>* left  = node_cur;
        Node<CalcNodeData>* right = pass_Power(expr);

        node_cur = new Node<CalcNodeData>;
        node_cur->setData({op_names[OP_POW], NODE_OPERATOR});
        node_cur->right_ = right;
        node_cur->left_  = left;
    }
    
    return node_cur;
}

//------------------------------------------------------------------------------

Node<CalcNodeData>* pass_Brackets (Expression& expr)
{
    if (*expr.symb_cur == '(')
    {
        ++expr.symb_cur;

        Node<CalcNodeData>* node_cur = pass_Plus_Minus(expr);

        CHECK_SYNTAX((*expr.symb_cur != ')'), CALC_SYNTAX_NO_CLOSE_BRACKET, expr, 1);
        ++expr.symb_cur;

        return node_cur;
    }

    else return pass_Function(expr);
}

//------------------------------------------------------------------------------

Node<CalcNodeData>* pass_Function (Expression& expr)
{
    if (isdigit(*expr.symb_cur)) return pass_Number(expr);

    else
    {
        CHECK_SYNTAX((not isalpha(*expr.symb_cur)), CALC_SYNTAX_ERROR, expr, 1);

        char* word = new char [MAX_STR_LEN] {};
        size_t index = 0;

        while (isalpha(*expr.symb_cur) || isdigit(*expr.symb_cur))
        {
            word[index++] = *expr.symb_cur;

            ++expr.symb_cur;
        }

        word[index] = '\0';

        if (*expr.symb_cur == '(')
        {
            int code = findFunc(word);
            Expression old = { expr.str, expr.symb_cur - index };
            CHECK_SYNTAX((code == 0), CALC_SYNTAX_UNIDENTIFIED_FUNCTION, old, index);

            Node<CalcNodeData>* arg = pass_Brackets(expr);

            Node<CalcNodeData>* node_cur = new Node<CalcNodeData>;
            node_cur->setData({op_names[code], NODE_FUNCTION});
            node_cur->right_ = arg;

            return node_cur;
        }
        else
        {
            Node<CalcNodeData>* node_cur = new Node<CalcNodeData>;
            node_cur->setData({ {0, word}, NODE_VARIABLE });

            return node_cur;
        }   
    }
}

//------------------------------------------------------------------------------

Node<CalcNodeData>* pass_Number (Expression& expr)
{
    double value = 0;
    char* begin = expr.symb_cur;

    value = strtod(expr.symb_cur, &expr.symb_cur);
    CHECK_SYNTAX((expr.symb_cur == begin), CALC_SYNTAX_NUMBER_ERROR, expr, 1);

    if (*expr.symb_cur == 'i') ++expr.symb_cur;

    char* number = new char [MAX_STR_LEN] {};
    memcpy(number, begin, expr.symb_cur - begin);

    Node<CalcNodeData>* node_cur = new Node<CalcNodeData>;
    node_cur->setData({ {0, number}, NODE_NUMBER });

    return node_cur;
}

//------------------------------------------------------------------------------

bool needBrackets (Node<CalcNodeData>* node, Node<CalcNodeData>* child)
{
    if  ( ((node-> getData().op.code == OP_MUL) || (node-> getData().op.code == OP_DIV)) &&
          ((child->getData().op.code == OP_ADD) || (child->getData().op.code == OP_SUB))   )
        return true;
    else
        return ( (node-> getData().op.code   == OP_POW)        &&
                 (child->getData().node_type == NODE_OPERATOR) &&
                 (child->getData().op.code   != OP_POW)          );
}

//------------------------------------------------------------------------------

char findFunc (char* word)
{
    assert(word != nullptr);

    operation func_key = { 0, word };

    operation* p_func_struct = (operation*)bsearch(&func_key, op_names, OP_NUM, sizeof(op_names[0]), CompareOP_Names);

    if (p_func_struct != nullptr) return p_func_struct->code;

    return 0;
}

//------------------------------------------------------------------------------

void Optimize (Tree<CalcNodeData>& tree)
{
    bool running = true;
    while (running)
    {
        running = Optimize(tree, tree.root_);
    }
    tree.root_->recountDepth();
}

//------------------------------------------------------------------------------

#define OPTIMIZE_ACTION(node_to_place)                   \
        {                                                \
            if (node_cur->prev_ == nullptr)              \
                tree.root_ = node_to_place;              \
            else                                         \
            if (node_cur->prev_->left_ == node_cur)      \
                node_cur->prev_->left_ = node_to_place;  \
            else                                         \
                node_cur->prev_->right_ = node_to_place; \
                                                         \
            node_to_place->prev_ = node_cur->prev_;      \
            node_to_place = nullptr;                     \
                                                         \
            node_cur->~Node();                           \
            delete node_cur;                             \
            return true;                                 \
        } //

//------------------------------------------------------------------------------

#define CALCULATE_ACTION(node, operation)                                              \
        {                                                                              \
            NUM_TYPE number1 = 0;                                                      \
            NUM_TYPE number2 = 0;                                                      \
                                                                                       \
            err = sscanf(node->left_ ->getData().op.word, NUM_PRINT_FORMAT, &number1); \
            assert(err);                                                               \
                                                                                       \
            err = sscanf(node->right_->getData().op.word, NUM_PRINT_FORMAT, &number2); \
            assert(err);                                                               \
                                                                                       \
            Node<CalcNodeData>* newnode = new Node<CalcNodeData>;                      \
            number1 = number1 operation number2;                                       \
                                                                                       \
            char* num_str = new char [MAX_STR_LEN] {};                                 \
            sprintf(num_str, "%.0lf", number1);                                        \
            newnode->setData({ {0, num_str}, NODE_NUMBER });                           \
            OPTIMIZE_ACTION(newnode);                                                  \
        } //

//------------------------------------------------------------------------------

bool Optimize (Tree<CalcNodeData>& tree, Node<CalcNodeData>* node_cur)
{
    assert(node_cur != nullptr);

    int err = 0;

    switch (node_cur->getData().node_type)
    {
    case NODE_FUNCTION:

        return Optimize(tree, node_cur->right_);
        break;

    case NODE_OPERATOR:

        switch (node_cur->getData().op.code)
        {
        case OP_ADD:
        case OP_SUB:

            if (node_cur->left_ == nullptr)
            {
                if (strcmp(node_cur->right_->getData().op.word, "0") == 0)
                {
                    OPTIMIZE_ACTION(node_cur->right_);
                }
                else return Optimize(tree, node_cur->right_);
            }
            else
            if (strcmp(node_cur->left_->getData().op.word, "0") == 0)
            {
                OPTIMIZE_ACTION(node_cur->right_);
            }
            else
            if (strcmp(node_cur->right_->getData().op.word, "0") == 0)
            {
                OPTIMIZE_ACTION(node_cur->left_);
            }
            else
            if ( (node_cur->left_ ->getData().node_type == NODE_NUMBER) &&
                 (node_cur->right_->getData().node_type == NODE_NUMBER)   )
            {
                CALCULATE_ACTION(node_cur, +);
            }
            else
            {
                if (Optimize(tree, node_cur->left_)) return true;

                return Optimize(tree, node_cur->right_);
            }
            break;

        case OP_MUL:

            if (strcmp(node_cur->left_->getData().op.word, "0") == 0)
            {
                OPTIMIZE_ACTION(node_cur->left_);
            }
            else
            if (strcmp(node_cur->right_->getData().op.word, "0") == 0)
            {
                OPTIMIZE_ACTION(node_cur->right_);
            }
            else
            if (strcmp(node_cur->left_->getData().op.word, "1") == 0)
            {
                OPTIMIZE_ACTION(node_cur->right_);
            }
            else
            if (strcmp(node_cur->right_->getData().op.word, "1") == 0)
            {
                OPTIMIZE_ACTION(node_cur->left_);
            }
            else
            if ( (node_cur->left_ ->getData().node_type == NODE_NUMBER) &&
                 (node_cur->right_->getData().node_type == NODE_NUMBER)   )
            {
                CALCULATE_ACTION(node_cur, *);
            }
            else
            {
                if (Optimize(tree, node_cur->left_)) return true;

                return Optimize(tree, node_cur->right_);
            }
            break;

        case OP_DIV:

            if (strcmp(node_cur->left_->getData().op.word, "0") == 0)
            {
                OPTIMIZE_ACTION(node_cur->left_);
            }
            else
            if (strcmp(node_cur->right_->getData().op.word, "1") == 0)
            {
                OPTIMIZE_ACTION(node_cur->left_);
            }
            else
            if ( (strcmp(node_cur->left_->getData().op.word, node_cur->right_->getData().op.word) == 0) &&
                 ((node_cur->left_->getData().node_type == NODE_VARIABLE) || (node_cur->left_->getData().node_type == NODE_NUMBER)) )
            {
                Node<CalcNodeData>* newnode = new Node<CalcNodeData>;
                newnode->setData({ {0, "1"}, NODE_NUMBER });

                OPTIMIZE_ACTION(newnode);
            }
            else
            {
                if (Optimize(tree, node_cur->left_)) return true;

                return Optimize(tree, node_cur->right_);
            }
            break;
        }
        break;

    case NODE_VARIABLE:
        break;
    
    case NODE_NUMBER:
        break;

    default: assert(0);
    }

    return false;
}

//------------------------------------------------------------------------------

void PrintBadExpr (const char* logname, const Expression& expr, size_t len)
{
    assert(logname != nullptr);

    FILE* log = fopen(logname, "a");
    assert(log != nullptr);

    fprintf(log, "\t %s\n", expr.str);
    printf (     "\t %s\n", expr.str);

    fprintf(log, "\t ");
    printf (     "\t ");

    for (int i = 0; i < expr.symb_cur - expr.str; ++i)
    {
        fprintf(log, " ");
        printf (     " ");
    }

    fprintf(log, "^");
    printf (     "^");

    for (int i = 0; i < len - 1; ++i)
    {
        fprintf(log, "~");
        printf (     "~");
    }

    fprintf(log, "\n");
    printf (     "\n");
}

//------------------------------------------------------------------------------
