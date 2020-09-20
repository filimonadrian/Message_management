#include "general_includes.h"

void usage(char *file);
bool tokenize_command(char buffer[], user_msg &message);
bool print_received_message(tcp_msg message);
