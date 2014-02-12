#!/bin/bash
sysctl -w net.core.rmem_max=2097152 net.core.rmem_default=1024
sysctl -w net.core.wmem_max=2097152 net.core.wmem_default=1024
