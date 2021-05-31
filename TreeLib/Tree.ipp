/*------------------------------------------------------------------------------
    * File:        Tree.ipp                                                    *
    * Description: Functions for binary trees.                                 *
    * Created:     18 apr 2021                                                 *
    * Author:      Artem Puzankov                                              *
    * Email:       puzankov.ao@phystech.edu                                    *
    * GitHub:      https://github.com/hellopuza                                *
    * Copyright © 2021 Artem Puzankov. All rights reserved.                    *
    *///------------------------------------------------------------------------

template <typename TYPE>
Node<TYPE>::Node () { }

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>::Tree () : errCode_ (TREE_NOT_CONSTRUCTED) { }

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>::Tree (char* tree_name) :
    name_         (tree_name),
    id_           (tree_id++),
    root_         (nullptr),
    path2badnode_ ((char*)"path to problem node"),
    errCode_      (TREE_OK)
{}

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>::Tree (char* tree_name, Node<TYPE>* root) :
    name_         (tree_name),
    root_         (root),
    id_           (tree_id++),
    path2badnode_ ((char*)"path to problem node"),
    errCode_      (TREE_OK)
{
    TREE_CHECK;
}

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>::Tree (char* tree_name, char* base_filename) :
    name_         (tree_name),
    id_           (tree_id++),
    path2badnode_ ((char*)"path to problem node"),
    errCode_      (TREE_OK)
{
    TREE_ASSERTOK((tree_name == nullptr), TREE_WRONG_INPUT_TREE_NAME);

    root_ = new Node<TYPE>;

    Text base(base_filename);

    TREE_ASSERTOK((base.num_ < 2), TREE_WRONG_SYNTAX_INPUT_BASE);
    TREE_ASSERTOK((strcmp(base.lines_[0].str,             OPEN_BRACKET ) != 0), TREE_WRONG_SYNTAX_INPUT_BASE);
    TREE_ASSERTOK((strcmp(base.lines_[base.num_ - 1].str, CLOSE_BRACKET) != 0), TREE_WRONG_SYNTAX_INPUT_BASE);

    size_t line_cur = 1;
    if (strcmp(base.lines_[line_cur].str, CLOSE_BRACKET) != 0)
    {
        if (root_->AddFromBase(base, line_cur) == TREE_WRONG_SYNTAX_INPUT_BASE)
        {
            PrintError(TREE_LOGNAME , __FILE__, __LINE__, __FUNC_NAME__, TREE_WRONG_SYNTAX_INPUT_BASE);
            PrintCode(base, line_cur, TREE_LOGNAME);
            exit(TREE_WRONG_SYNTAX_INPUT_BASE);
        };
    }

    TREE_CHECK;
}

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>::Tree (const Tree& obj)
{
    *this = obj;
}

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>& Tree<TYPE>::operator = (const Tree& obj)
{
    TREE_CHECK;

    if (obj.root_ != nullptr)
    {
        if (root_ == nullptr) root_ = new Node<TYPE>;
        *root_ = *obj.root_;
    }
    else root_ = nullptr;

    return *this;
}

//------------------------------------------------------------------------------

template <typename TYPE>
Tree<TYPE>::~Tree ()
{
    if (errCode_ == TREE_NOT_CONSTRUCTED) {}

    else if (errCode_ != TREE_DESTRUCTED)
    {
        if (root_ != nullptr)
        {
            root_->~Node();
            delete root_;
            root_ = nullptr;
        }

        errCode_ = TREE_DESTRUCTED;
    }
    else
    {
        TREE_ASSERTOK(TREE_DESTRUCTOR_REPEATED, TREE_DESTRUCTOR_REPEATED);
    }
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Tree<TYPE>::Clean ()
{
    if (root_ != nullptr)
    {
        root_->~Node();
        delete root_;
        root_ = nullptr;
    }
}

//------------------------------------------------------------------------------

template <typename TYPE>
Node<TYPE>::Node (const Node& obj)
{
    *this = obj;
}

//------------------------------------------------------------------------------

template <typename TYPE>
Node<TYPE>& Node<TYPE>::operator = (const Node& obj)
{
    if constexpr (std::is_same<TYPE, char*>::value)
        if (is_dynamic_)
            delete [] data_;

    if (obj.is_dynamic_)
        if constexpr (std::is_same<TYPE, char*>::value)
            data_ = new char[strlen(obj.data_) + 2] {};

    copyType(data_, obj.data_);
    is_dynamic_ = obj.is_dynamic_;

    if (obj.right_ != nullptr)
    {
        if (right_ != nullptr) right_->~Node();
        right_ = new Node<TYPE>;

        *right_ = *obj.right_;
        right_->prev_ = this;
    }
    else
    {
        if (right_ != nullptr) right_->~Node();
        right_ = nullptr;
    }
    
    if (obj.left_ != nullptr)
    {
        if (left_ != nullptr) left_->~Node();
        left_ = new Node<TYPE>;

        *left_ = *obj.left_;
        left_->prev_ = this;
    }
    else
    {
        if (left_ != nullptr) left_->~Node();
        left_ = nullptr;
    }

    if (prev_ == nullptr) depth_ = 0;
    else depth_ = prev_->depth_ + 1;

    return *this;
}

//------------------------------------------------------------------------------

template <typename TYPE>
Node<TYPE>::~Node ()
{
    if (right_ != nullptr)
    {
        right_->~Node();
        delete right_;
        right_ = nullptr;
    }

    if (left_ != nullptr)
    {
        left_->~Node();
        delete left_;
        left_  = nullptr;
    }

    prev_ = nullptr;

    if constexpr (std::is_same<TYPE, char*>::value)
        if (is_dynamic_)
            delete [] data_;

    is_dynamic_ = false;

    data_ = POISON<TYPE>;
}

//------------------------------------------------------------------------------

template <typename TYPE>
int Node<TYPE>::AddFromBase (const Text& base, size_t& line_cur)
{
    assert(line_cur < base.num_);

    if (strcmp(base.lines_[line_cur].str, CLOSE_BRACKET) == 0) return 0;

    if constexpr (std::is_same<TYPE, char*>::value)
    {
        data_ = new char [base.lines_[line_cur].len + 2];
        strcpy(data_, base.lines_[line_cur].str);
        ++line_cur;

        is_dynamic_ = true;
    }
    else
        sscanf(base.lines_[line_cur++].str, PRINT_FORMAT<TYPE>, &data_);

    if (strcmp(base.lines_[line_cur].str, OPEN_BRACKET) == 0)
    {
        right_ = new Node<TYPE>;
        right_->prev_ = this;
        right_->depth_ = depth_ + 1;

        int err = right_->AddFromBase(base, ++line_cur);
        if (err) return err;
        ++line_cur;
    }
    else if (strcmp(base.lines_[line_cur].str, CLOSE_BRACKET) == 0) return 0;
    else return TREE_WRONG_SYNTAX_INPUT_BASE;

    if (strcmp(base.lines_[line_cur].str, OPEN_BRACKET) == 0)
    {
        left_ = new Node<TYPE>;
        left_->prev_ = this;
        left_->depth_ = depth_ + 1;

        int err = left_->AddFromBase(base, ++line_cur);
        if (err) return err;
        ++line_cur;
    }

    return 0;
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Tree<TYPE>::Dump (const char* dumpname)
{
    FILE* dump = fopen(dumpname, "w");
    assert(dump != nullptr);

    fprintf(dump, "digraph G{\n" "rankdir = HR;\n node[shape=box];\n");

    root_->Dump(dump);

    fprintf(dump, "\tlabelloc=\"t\";"
                  "\tlabel=\"Tree name: %s\\nType is %s\";"
                  "}\n", name_, PRINT_TYPE<TYPE>);

    fclose(dump);

    char command[128] = "";

    sprintf(command, "win_iconv -f 1251 -t UTF8 \"%s\" > \"new%s\"", dumpname, dumpname);
    system(command);

    sprintf(command, "dot -Tpng -o %s new%s", DUMP_PICT_NAME, dumpname);
    system(command);

    sprintf(command, "del new%s", dumpname);
    system(command);
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Node<TYPE>::Dump (FILE* dump)
{
    assert(dump != nullptr);
    
    fprintf(dump, "\t \"prev: 0x%p\\n", prev_);
    fprintf(dump, " this: 0x%p\\n depth: %u\\n data: [", this, depth_);
    TypePrint(dump, data_);
    fprintf(dump, "]\\n left: 0x%p | right: 0x%p\\n", left_, right_);
    fprintf(dump, "\" [shape = box, style = filled, color = black, fillcolor = lightskyblue]\n");

    if (left_ != nullptr)
    {
        fprintf(dump, "\t \"prev: 0x%p\\n", prev_);
        fprintf(dump, " this: 0x%p\\n depth: %u\\n data: [", this, depth_);
        TypePrint(dump, data_);
        fprintf(dump, "]\\n left: 0x%p | right: 0x%p\\n", left_, right_);

        fprintf(dump, "\" -> \"");

        fprintf(dump, "prev: 0x%p\\n", left_->prev_);
        fprintf(dump, " this: 0x%p\\n depth: %u\\n data: [", left_, left_->depth_);
        TypePrint(dump, left_->data_);
        fprintf(dump, "]\\n left: 0x%p | right: 0x%p\\n", left_->left_, left_->right_);
        fprintf(dump, "\" [label=\"left\"]\n");
    }

    if (right_ != nullptr)
    {
        fprintf(dump, "\t \"prev: 0x%p\\n", prev_);
        fprintf(dump, " this: 0x%p\\n depth: %u\\n data: [", this, depth_);
        TypePrint(dump, data_);
        fprintf(dump, "]\\n left: 0x%p | right: 0x%p\\n", left_, right_);

        fprintf(dump, "\" -> \"");

        fprintf(dump, "prev: 0x%p\\n", right_->prev_);
        fprintf(dump, " this: 0x%p\\n depth: %u\\n data: [", right_, right_->depth_);
        TypePrint(dump, right_->data_);
        fprintf(dump, "]\\n left: 0x%p | right: 0x%p\\n", right_->left_, right_->right_);
        fprintf(dump, "\" [label=\"right\"]\n");
    }


    if (left_  != nullptr) left_->Dump(dump);
    if (right_ != nullptr) right_->Dump(dump);
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Tree<TYPE>::Write (const char* basename)
{
    FILE* base = fopen(basename, "w");
    assert(base != nullptr);

    fprintf(base, "[\n");
    if (root_ != nullptr) root_->Write(base);
    fprintf(base, "]");

    fclose(base);
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Node<TYPE>::Write (FILE* base)
{
    assert(base != nullptr);

    for (int i = 0; i <= depth_; ++i) fprintf(base, "    ");
    TypePrint(base, data_);
    fprintf(base, "\n");

    if (right_ != nullptr)
    {
        for (int i = 0; i <= depth_; ++i) fprintf(base, "    ");
        fprintf(base, "[\n");

        right_->Write(base);
        
        for (int i = 0; i <= depth_; ++i) fprintf(base, "    ");
        fprintf(base, "]\n");
    }

    if (left_ != nullptr)
    {
        for (int i = 0; i <= depth_; ++i) fprintf(base, "    ");
        fprintf(base, "[\n");

        if (left_ != nullptr) left_->Write(base);

        for (int i = 0; i <= depth_; ++i) fprintf(base, "    ");
        fprintf(base, "]\n");
    }
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Node<TYPE>::setData (TYPE data)
{
    if constexpr (std::is_same<TYPE, char*>::value)
    {
        if (is_dynamic_)
            delete [] data_;

        data_ = new char [strlen(data) + 2] {};
        is_dynamic_ = true;
    }

    copyType(data_, data);
}

//------------------------------------------------------------------------------

template <typename TYPE>
const TYPE& Node<TYPE>::getData ()
{
    return data_;
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Node<TYPE>::recountDepth ()
{
    assert(this != nullptr);

    if (prev_ == nullptr)
        depth_ = 0;
    else
        depth_ = prev_->depth_ + 1;

    if (right_ != nullptr) right_->recountDepth();
    if (left_  != nullptr) left_->recountDepth();
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Node<TYPE>::recountPrev ()
{
    assert(this != nullptr);

    if (right_ != nullptr)
    {
        right_->prev_ = this;
        right_->recountPrev();
    }
    if (left_  != nullptr)
    {
        left_->prev_ = this;
        left_->recountPrev();
    }
}

//------------------------------------------------------------------------------

template <typename TYPE>
int Tree<TYPE>::findPath (Stack<size_t>& path, TYPE elem)
{
    TREE_ASSERTOK((isPOISON(elem)), TREE_INPUT_DATA_POISON);

    bool found = root_->findPath(path, elem);

    return found;
}

//------------------------------------------------------------------------------

template <typename TYPE>
int Node<TYPE>::findPath (Stack<size_t>& path, TYPE elem)
{
    path.Push((size_t)this);
    
    bool found = false;

    if (right_ != nullptr)
    {
        found = right_->findPath(path, elem);
        if (found) return found;
    }
    if (left_ != nullptr)
    {
        found = left_->findPath(path, elem);
        if (found) return found;
    }

    if ((left_ == nullptr) && (right_ == nullptr))
    {
        if constexpr (std::is_same<TYPE, char*>::value)
            found = (strcmp(elem, data_) == 0);
        else
            found = (elem == data_);
    }

    if (not found) path.Pop();

    return found;
}

//------------------------------------------------------------------------------

template <typename TYPE>
int Tree<TYPE>::Check ()
{
    int err = TREE_OK;

    if (root_ != nullptr)
        err = root_->Check(*this);

    errCode_ = err;

    return err;
}

//------------------------------------------------------------------------------

template <typename TYPE>
int Node<TYPE>::Check (Tree<TYPE>& tree)
{
    if (((prev_ == nullptr) && (depth_ != 0)) ||
        ((prev_ != nullptr) && (depth_ != prev_->depth_ + 1)))
    {
        tree.path2badnode_.Push(data_);
        return TREE_WRONG_DEPTH;
    }

    if (prev_ != nullptr)
        if ((prev_->right_ != this) &&
            (prev_->left_ != this))
        {
            tree.path2badnode_.Push(data_);
            return TREE_WRONG_PREV_NODE;
        }

    if (right_ != nullptr)
        if (right_->prev_ != this)
        {
            tree.path2badnode_.Push(data_);
            return TREE_WRONG_PREV_NODE;
        }

    if (left_ != nullptr)
        if (left_->prev_ != this)
        {
            tree.path2badnode_.Push(data_);
            return TREE_WRONG_PREV_NODE;
        }

    int err = TREE_OK;

    if (right_ != nullptr)
        err = right_->Check(tree);

    if (err)
    {
        tree.path2badnode_.Push(data_);
        return err;
    }

    if (left_ != nullptr)
        err = left_->Check(tree);

    if (err) tree.path2badnode_.Push(data_);

    return err;
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Tree<TYPE>::PrintError (const char* logname, const char* file, int line, const char* function, int err)
{
    assert(function != nullptr);
    assert(logname  != nullptr);
    assert(file     != nullptr);

    FILE* log = fopen(logname, "a");
    assert(log != nullptr);

    fprintf(log, "********************************************************************************\n");

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log, "TIME: %d-%02d-%02d %02d:%02d:%02d\n\n",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);

    fprintf(log, "ERROR: file %s  line %d  function %s\n\n", file, line, function);
    fprintf(log, "%s\n", tree_errstr[err + 1]);

    if (path2badnode_.getSize() != 0)
    {
        fprintf(log, "%s", path2badnode_.getName());
        for (int i = path2badnode_.getSize() - 1; i > -1; --i)
        {
            fprintf(log, " -> [");
            TypePrint(log, path2badnode_[i]);
            fprintf(log, "]");
        }

        fprintf(log, "\n");
    }
    if (err != TREE_WRONG_SYNTAX_INPUT_BASE) fprintf(log, "You can look tree dump in %s\n\n", DUMP_PICT_NAME);
    fclose(log);

    ////

    printf("ERROR: file %s  line %d  function %s\n", file, line, function);
    printf("%s\n\n", tree_errstr[err + 1]);

    if (path2badnode_.getSize() != 0)
    {
        printf("%s", path2badnode_.getName());
        for (int i = path2badnode_.getSize() - 1; i > -1; --i)
        {
            printf(" -> [");
            TypePrint(stdout, path2badnode_[i]);
            printf("]");
        }

        printf("\n");
    }
    if (err != TREE_WRONG_SYNTAX_INPUT_BASE) printf (     "You can look tree dump in %s\n\n", DUMP_PICT_NAME);
}

//------------------------------------------------------------------------------

template <typename TYPE>
void Tree<TYPE>::PrintCode (Text& base, size_t line, const char* logname)
{
    assert(logname != nullptr);

    FILE* log = fopen(logname, "a");
    assert(log != nullptr);

    fprintf(log, "\n");
    printf (     "\n");

    fprintf(log, "////////////////--TEXT-SECTION--////////////////" "\n");
    printf (     "////////////////--TEXT-SECTION--////////////////" "\n");

    size_t true_line = line + 1;
    
    for (int i = -2; i <= 2; ++i)
    {
        if ((true_line + i > 0) && (true_line + i <= base.num_))
        {
            fprintf(log, "%s%5d: %s\n", ((i == 0)? "=>" : "  "), true_line + i, base.lines_[true_line + i - 1].str);
            printf (     "%s%5d: %s\n", ((i == 0)? "=>" : "  "), true_line + i, base.lines_[true_line + i - 1].str);
        }
    }

    fprintf(log, "////////////////////////////////////////////////" "\n\n");
    printf (     "////////////////////////////////////////////////" "\n\n");

    fclose(log);
}

//------------------------------------------------------------------------------
