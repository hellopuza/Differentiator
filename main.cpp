/*------------------------------------------------------------------------------
    * File:        main.cpp                                                    *
    * Description: Program for calculating derivatives of functions.           *
    * Created:     15 may 2021                                                 *
    * Author:      Artem Puzankov                                              *
    * Email:       puzankov.ao@phystech.edu                                    *
    * GitHub:      https://github.com/hellopuza                                *
    * Copyright © 2021 Artem Puzankov. All rights reserved.                    *
    *///------------------------------------------------------------------------

#include "Differentiator.h"

//------------------------------------------------------------------------------

int main (int argc, char* argv[])
{
    if (argc == 1)
    {
        Differentiator diff;

        return diff.Run();
    }
    else
    {
        Differentiator diff(argv[1]);

        return diff.Run();
    }

    return 0;
}
