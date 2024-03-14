#pragma once

#include <vector>
#include <memory>

template<typename FrameT>
class FramePipeline
{
public:
    class IComponent
    {
    public:
        virtual std::shared_ptr<FrameT> process_frame(const std::shared_ptr<FrameT>&) = 0;
    };

    struct ComponentHolder
    {
        IComponent *ptr;
        bool should_free;
    };

    FramePipeline() = default;

    void add_component(IComponent *component)
    {
        _components.push_back({component, false});
    }

    template<typename ComponentT, typename... Args>
    void make_component(Args&&... args)
    {
        _components.push_back({new ComponentT(std::forward(args)...), true});
    }

    std::shared_ptr<FrameT> process_frame(std::shared_ptr<FrameT> frame)
    {
        for (auto &[component, _] : _components)
        {
            frame = component->process_frame(frame);
        }

        return frame;
    }

private:
    std::vector<ComponentHolder> _components;
};

