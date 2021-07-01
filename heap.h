#pragma once

#define halloc_type(t, c) (t *)halloc(sizeof (t) * (c))
#define halloc_clear_type(t, c) halloc_clear(sizeof (t) * (c))

void *halloc(const size_t);
void *halloc_clear(const size_t);
void hfree(void *);
