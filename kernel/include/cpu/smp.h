#pragma once

#include <stivale2.h>
#include <typedefs.h>

void ap_init(u64 lapic_id);
void smp_init(struct stivale2_struct_tag_smp *);
