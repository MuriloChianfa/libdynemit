#include <stdio.h>
#include <dynemit.h>

int main(void)
{
    // Test the features API
    printf("Testing dynemit_features() API:\n");
    printf("=================================\n\n");
    
    const char **features = dynemit_features();
    printf("Available features in this build:\n");
    for (int i = 0; features[i] != nullptr; i++) {
        printf("  [%d] %s\n", i, features[i]);
    }
    
    printf("\n");
    
    // Also test SIMD detection
    simd_level_t level = detect_simd_level();
    printf("Detected SIMD level: %s\n", simd_level_name(level));
    
    return 0;
}
