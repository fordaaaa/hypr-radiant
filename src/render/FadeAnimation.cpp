#include <hypr-radiant/render/FadeAnimation.hpp>

#include <algorithm>
#include <cmath>

namespace hypr_radiant {

double easedAnimationProgress(AnimationCurve curve, double linear, bool opening) noexcept {
    const auto progress = std::clamp(linear, 0.0, 1.0);
    switch (curve) {
    case AnimationCurve::Quattro: {
        // Quattro's image carousel snaps confidently into its final pose but leaves gently: an
        // asymmetric cubic makes opening feel immediate without making dismissal look abrupt.
        const auto inverse = 1.0 - progress;
        return opening ? 1.0 - inverse * inverse * inverse : progress * progress * progress;
    }
    case AnimationCurve::Cyberpunk: {
        // A controlled digital cut: visible steps give the overview a glitch cadence while the
        // continuous component keeps pointer-driven and interrupted transitions responsive.
        constexpr auto steps = 7.0;
        const auto quantized = progress >= 1.0 ? 1.0 : static_cast<double>(static_cast<int>(progress * steps)) / steps;
        return quantized * 0.72 + progress * 0.28;
    }
    case AnimationCurve::Tron: {
        // A precise light sweep: fast ignition on arrival, then a measured power-down on exit.
        const auto inverse = 1.0 - progress;
        return opening ? 1.0 - inverse * inverse : progress * progress;
    }
    case AnimationCurve::Elegant:
        // Quintic smoothstep has zero velocity and acceleration at both ends, producing the
        // restrained, polished motion expected from the quietest profile.
        return progress * progress * progress * (progress * (progress * 6.0 - 15.0) + 10.0);
    case AnimationCurve::Smooth:
        return progress * progress * (3.0 - 2.0 * progress);
    }
    return progress;
}

void FadeAnimation::animateTo(bool visible, int durationMs) {
    const auto now = Clock::now();
    update(now);

    m_startedAt     = now;
    m_duration      = std::chrono::milliseconds{std::max(0, durationMs)};
    m_startValue    = m_currentValue;
    m_targetValue   = visible ? 1.0 : 0.0;
    m_targetVisible = visible;
    m_running       = m_duration.count() > 0 && std::abs(m_targetValue - m_startValue) > 0.001;

    if (!m_running)
        m_currentValue = m_targetValue;
}

void FadeAnimation::hideImmediate() {
    m_startValue    = 0.0;
    m_currentValue  = 0.0;
    m_targetValue   = 0.0;
    m_targetVisible = false;
    m_running       = false;
    m_duration      = std::chrono::milliseconds{0};
}

void FadeAnimation::setProgress(double value, bool targetVisible) {
    m_currentValue = std::clamp(value, 0.0, 1.0);
    m_startValue = m_currentValue;
    m_targetValue = m_currentValue;
    m_targetVisible = targetVisible;
    m_running = false;
    m_duration = std::chrono::milliseconds{0};
}

void FadeAnimation::setCurve(AnimationCurve curve) noexcept {
    m_curve = curve;
}

double FadeAnimation::value() {
    update(Clock::now());
    return m_currentValue;
}

bool FadeAnimation::targetVisible() const noexcept {
    return m_targetVisible;
}

bool FadeAnimation::running() {
    update(Clock::now());
    return m_running;
}

bool FadeAnimation::renderable() {
    update(Clock::now());
    return m_running || m_currentValue > 0.001;
}

void FadeAnimation::update(Clock::time_point now) {
    if (!m_running)
        return;

    const auto elapsed = std::chrono::duration<double, std::milli>(now - m_startedAt).count();
    const auto total   = static_cast<double>(m_duration.count());

    if (total <= 0.0 || elapsed >= total) {
        m_currentValue = m_targetValue;
        m_running      = false;
        return;
    }

    const auto linear = std::clamp(elapsed / total, 0.0, 1.0);
    const auto eased  = easedAnimationProgress(m_curve, linear, m_targetValue >= m_startValue);

    m_currentValue = std::lerp(m_startValue, m_targetValue, eased);
}

} // namespace hypr_radiant
