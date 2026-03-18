#pragma once
#include <JuceHeader.h>

class CustomTheme : public juce::LookAndFeel_V4
{
public:
    CustomTheme()
    {
        // Colores base para todos los Sliders del programa
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff56b6c2)); // Bolita Cyan
        setColour(juce::Slider::trackColourId, juce::Colour(0xff2c313a)); // Riel oscuro
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff181a1f));
    }

    // 1. DIBUJAMOS LOS BOTONES REDONDEADOS
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f, 1.0f);
        
        // Calculamos el color dependiendo si le pasas el mouse o le das clic
        auto baseColour = backgroundColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
        if (isButtonDown || button.getToggleState())  baseColour = baseColour.brighter(0.2f);
        else if (isMouseOverButton)                   baseColour = baseColour.brighter(0.1f);

        // Pintamos el fondo curvo (5px de radio para ese look moderno)
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 5.0f); 

        // Un pequeño contorno súper sutil para dar volumen
        g.setColour(juce::Colour(0x33ffffff));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }

    // 2. DIBUJAMOS LOS SLIDERS MODERNOS TIPO SPLICE/ARCADE
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto trackWidth = 4.0f; // Un riel muy delgadito y elegante
        juce::Point<float> startPoint(slider.isHorizontal() ? (float)x : (float)x + (float)width * 0.5f,
                                      slider.isHorizontal() ? (float)y + (float)height * 0.5f : (float)(height + y));

        auto trackBounds = slider.isHorizontal() ? juce::Rectangle<float>(static_cast<float>(x), startPoint.y - trackWidth * 0.5f, static_cast<float>(width), trackWidth)
                                                 : juce::Rectangle<float>(startPoint.x - trackWidth * 0.5f, static_cast<float>(y), trackWidth, static_cast<float>(height));

        // Dibujar riel vacío
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(trackBounds, trackWidth * 0.5f);

        // Dibujar riel lleno (Cyan)
        auto activeTrackBounds = slider.isHorizontal() ? trackBounds.withWidth(sliderPos - trackBounds.getX())
                                                       : trackBounds.withTop(sliderPos).withBottom(trackBounds.getBottom());
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillRoundedRectangle(activeTrackBounds, trackWidth * 0.5f);

        // Dibujar la "bolita" (Thumb)
        g.setColour(slider.findColour(juce::Slider::thumbColourId).brighter(0.2f));
        auto thumbWidth = 12.0f;
        juce::Rectangle<float> thumbBounds(slider.isHorizontal() ? sliderPos - thumbWidth * 0.5f : startPoint.x - thumbWidth * 0.5f,
                                           slider.isHorizontal() ? startPoint.y - thumbWidth * 0.5f : sliderPos - thumbWidth * 0.5f,
                                           thumbWidth, thumbWidth);
        g.fillEllipse(thumbBounds);
    }

    // 3. SCROLLBARS MODERNOS Y ELEGANTES
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
        bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override
    {
        g.fillAll(juce::Colour(0xff121418)); // Fondo del riel oscuro

        juce::Rectangle<float> thumbBounds;
        if (isScrollbarVertical)
            thumbBounds = { (float)x + 2.0f, (float)thumbStartPosition, (float)width - 4.0f, (float)thumbSize };
        else
            thumbBounds = { (float)thumbStartPosition, (float)y + 2.0f, (float)thumbSize, (float)height - 4.0f };

        g.setColour(juce::Colour(0xff3e4451)); // Color de la barra
        if (isMouseOver || isMouseDown) g.setColour(juce::Colour(0xff5c6370)); // Brilla al pasar el mouse

        g.fillRoundedRectangle(thumbBounds, 4.0f); // Bordes redondeados
    }

    // 4. CAJAS DE BUSCAR/BPM REDONDEADAS
    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override
    {
        g.setColour(juce::Colour(0xff121418)); // Fondo oscuro
        g.fillRoundedRectangle(0, 0, width, height, 5.0f); // 5px de curva
    }
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override
    {
        g.setColour(textEditor.hasKeyboardFocus(true) ? juce::Colour(0xff56b6c2) : juce::Colour(0xff2c313a));
        g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, 5.0f, 1.0f);
    }

    // 5. COMBOBOX (Selector de Key) REDONDEADO
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        g.setColour(juce::Colour(0xff121418));
        g.fillRoundedRectangle(0, 0, width, height, 5.0f);

        g.setColour(juce::Colour(0xff2c313a));
        g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, 5.0f, 1.0f);

        // Dibujamos la flechita
        juce::Path path;
        path.addTriangle(buttonX + buttonW * 0.3f, buttonY + buttonH * 0.4f,
            buttonX + buttonW * 0.7f, buttonY + buttonH * 0.4f,
            buttonX + buttonW * 0.5f, buttonY + buttonH * 0.6f);
        g.setColour(juce::Colour(0xffabb2bf));
        g.fillPath(path);
    }

    // 6. FONDO DEL MENÚ DESPLEGABLE (PopupMenu)
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        // Limpiamos el fondo feo por defecto
        g.fillAll(juce::Colours::transparentBlack);

        juce::Rectangle<float> bounds(0, 0, (float)width, (float)height);

        // Pintamos el fondo oscuro unificado (igual que el panel lateral)
        g.setColour(juce::Colour(0xff1c1f26));
        g.fillRoundedRectangle(bounds, 4.0f); // 4px es curvo, pero no demasiado

        // Un borde muy sutil para que no se pierda contra el fondo
        g.setColour(juce::Colour(0xff2c313a));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    }

    // 7. ELEMENTOS DE LA LISTA (Los textos y el hover)
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
        const bool isSeparator, const bool isActive,
        const bool isHighlighted, const bool isTicked,
        const bool hasSubMenu, const juce::String& text,
        const juce::String& shortcutKeyText,
        const juce::Drawable* icon, const juce::Colour* const textColourToUse) override
    {
        auto textColour = juce::Colour(0xffabb2bf); // Color base de las letras
        auto bgRect = area.toFloat();

        // Efecto "Hover" al pasar el mouse
        if (isHighlighted && isActive)
        {
            g.setColour(juce::Colour(0xff2c313a));
            g.fillRoundedRectangle(bgRect.reduced(2.0f), 4.0f); // Respetamos las curvas
            textColour = juce::Colours::white;
        }

        g.setColour(textColour);
        g.setFont(14.0f); // Fuente minimalista

        auto textRect = area.reduced(25, 0); // Dejamos espacio a la izquierda para la marca

        // Si la opción está seleccionada, dibujamos un check estilizado
        if (isTicked)
        {
            g.setColour(juce::Colour(0xff56b6c2)); // Tu Cyan premium
            juce::Path tick;
            tick.startNewSubPath(8.0f, (float)area.getCentreY() - 1.0f);
            tick.lineTo(13.0f, (float)area.getCentreY() + 4.0f);
            tick.lineTo(21.0f, (float)area.getCentreY() - 5.0f);
            g.strokePath(tick, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
        }

        g.drawText(text, textRect, juce::Justification::centredLeft, true);
    }
};