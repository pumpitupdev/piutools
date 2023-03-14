#pragma once

char* fsr_redirect_path(const char* original_path);
void init_fsr(const char* fsr_root, const char* fsr_redirect_paths);