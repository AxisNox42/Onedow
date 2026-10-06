#pragma once

class TextRenderer;

// Shared localized text renderers injected into UI scenes.
struct SceneTextContext {
    TextRenderer& title;
    TextRenderer& body;
};
