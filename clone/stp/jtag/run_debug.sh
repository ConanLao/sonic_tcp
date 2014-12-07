#!/bin/sh
export QUARTUS_HOME=/opt/altera/13.1/quartus
export SYSTEM_CONSOLE_PATH=/sopc_builder/bin/
export SYS_CONSOLE_SCRIPT_PATH=.

echo "RUN 'source init' to start"
$QUARTUS_HOME/$SYSTEM_CONSOLE_PATH/system-console -cli
