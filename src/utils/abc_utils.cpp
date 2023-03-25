#include <stdio.h>
#include "abc_utils.h"

int blif_file_to_binary_aig_file(std::string& blifFile, std::string& aigerFile) {
    char* Command;
    Abc_Frame_t * pAbc;
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    if ( Cmd_CommandExecute( pAbc, ("read " + blifFile).c_str() ) )
    {
        fprintf( stdout, "Cannot read blif path %s", blifFile.c_str() );
        return EXIT_FAILURE;
    }

    // Make the circle smaller
    Cmd_CommandExecute( pAbc, "strash" );
    Cmd_CommandExecute( pAbc, "rewrite" ); Cmd_CommandExecute( pAbc, "balance" ); Cmd_CommandExecute( pAbc, "refactor" );
    Cmd_CommandExecute( pAbc, "rewrite" ); Cmd_CommandExecute( pAbc, "balance" ); Cmd_CommandExecute( pAbc, "refactor" );
    Cmd_CommandExecute( pAbc, "rewrite" ); Cmd_CommandExecute( pAbc, "balance" ); Cmd_CommandExecute( pAbc, "refactor" );

    // Write the AIGER file
    if ( Cmd_CommandExecute( pAbc, ("write " + aigerFile).c_str() ) )
    {
        fprintf( stdout, "Cannot write AIGER path %s", aigerFile.c_str() );
        return EXIT_FAILURE;
    }

    Abc_Stop();
    return EXIT_SUCCESS;
}
