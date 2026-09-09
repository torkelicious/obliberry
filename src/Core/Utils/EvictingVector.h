#include <cstddef>
#include <vector>
namespace Containers {

    template <typename T> class EvictingVector {
        explicit EvictingVector(size_t max_size) : m_capacity(max_size) {}

    public:
        // prune oldests
        void push_back(T item) {
            if (m_data.size() >= m_capacity) {
                m_data.erase(m_data.begin());
            }
            m_data.push_back(std::move(item));
        }
        size_t size() { return m_data.size(); }
        const std::vector<T> &data() const { return m_data; }

    private:
        size_t m_capacity;
        std::vector<T> m_data;
    };

} // namespace Containers
