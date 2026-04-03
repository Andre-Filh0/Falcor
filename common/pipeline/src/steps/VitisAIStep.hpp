// OBSOLETO — substituido por InferenceStep + VitisAIBackend.
//
// Use:
//   #include "inference/InferenceStep.hpp"
//   #include "inference/backends/VitisAIBackend.hpp"
//
// Exemplo (src/main.cpp):
//   pipe->addStep(std::make_unique<InferenceStep>(
//       std::make_unique<VitisAIBackend>(),
//       "cattle_weight.xmodel", 224, 224,
//       [](const std::vector<float>& out, FrameMetadata& meta) {
//           meta.weight_kg = out[0] * 1000.0f;
//       }
//   ));
