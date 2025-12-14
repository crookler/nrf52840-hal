define kwit
mon tpwr disable
det
q
end

set arch armv7e-m
set mem inaccessible-by-default off
file <template-file>
tar ext <template-port>
mon swd_scan
att 1
mon rtt ident <template-rttid>
mon rtt enable
mon vector_catch disable reset
load
run