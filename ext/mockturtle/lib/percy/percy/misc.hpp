#pragma once

#include <algorithm>

namespace percy
{

	inline unsigned binomial_coeff(int n, int k)
	{
		auto C = new unsigned*[n + 1];
		for (int i = 0; i < n + 1; i++) {
			C[i] = new unsigned[k + 1];
		}

		for (int i = 0; i <= n; i++) {
			for (int j = 0; j <= (std::min)(i, k); j++)
			{
				if (j == 0 || j == i) {
					C[i][j] = 1;
				} else {
					C[i][j] = C[i-1][j-1] + C[i-1][j];
				}
			}
		}
		const auto result = C[n][k];

		for (int i = 0; i < n + 1; i++) {
			delete[] C[i];
		}
		delete[] C;

		return result;
	}

    /***************************************************************************
            Returns 1 if fanins1 > fanins2, -1 if fanins1 < fanins2, and 0
            otherwise. (In co-lexicographical order.)
    ***************************************************************************/
    template<typename fanin, int FI>
    int colex_compare(
            const std::array<fanin, FI>& fanins1,
            const std::array<fanin, FI>& fanins2)
    {
        for (int i = FI-1; i >= 0; i--) {
            if (fanins1[i] < fanins2[i]) {
                return -1;
            } else if (fanins1[i] > fanins2[i]) {
                return 1;
            }
        }

        // All fanins are equal
        return 0;
    }

    inline int colex_compare(const int* const fanins1, const int* const fanins2, int fanin)
    {
        for (int i = fanin-1; i >= 0; i--) {
            if (fanins1[i] < fanins2[i]) {
                return -1;
            } else if (fanins1[i] > fanins2[i]) {
                return 1;
            }
        }

        // All fanins are equal
        return 0;
    }

    inline int colex_compare(const std::vector<int>& fanins1, const std::vector<int>& fanins2)
    {
        assert(fanins1.size() == fanins2.size());
        for (int i = fanins1.size() - 1; i >= 0; i--) {
            if (fanins1[i] < fanins2[i]) {
                return -1;
            } else if (fanins1[i] > fanins2[i]) {
                return 1;
            }
        }

        // All fanins are equal
        return 0;
    }

    /***************************************************************************
            Returns 1 if fanins1 > fanins2, -1 if fanins1 < fanins2, and 0
            otherwise. (In lexicographical order.)
    ***************************************************************************/
    template<typename fanin, int FI>
    int lex_compare(
            const std::array<fanin, FI>& fanins1,
            const std::array<fanin, FI>& fanins2)
    {
        for (int i = 0; i < FI; i++) {
            if (fanins1[i] < fanins2[i]) {
                return -1;
            } else if (fanins1[i] > fanins2[i]) {
                return 1;
            }
        }

        // All fanins are equal
        return 0;
    }

    template<typename fanin, int FI>
    int lex_compare(const fanin* const fanins1, const fanin* const fanins2)
    {
        for (int i = 0; i < FI; i++) {
            if (fanins1[i] < fanins2[i]) {
                return -1;
            } else if (fanins1[i] > fanins2[i]) {
                return 1;
            }
        }

        // All fanins are equal
        return 0;
    }

    inline int lex_compare(const std::vector<int>& fanins1, const std::vector<int>& fanins2)
    {
        assert(fanins1.size() == fanins2.size());

        for (auto i = 0u; i < fanins1.size(); i++) {
            if (fanins1[i] < fanins2[i]) {
                return -1;
            } else if (fanins1[i] > fanins2[i]) {
                return 1;
            }
        }

        // All fanins are equal
        return 0;
    }

    inline int lex_compare(const std::array<int, 3>& fanins1, const std::array<int, 3>& fanins2)
    {
        if (fanins1[0] < fanins2[0]) {
            return -1;
        } else if (fanins1[0] > fanins2[0]) {
            return 1;
        }
        if (fanins1[1] < fanins2[1]) {
            return -1;
        } else if (fanins1[1] > fanins2[1]) {
            return 1;
        }
        if (fanins1[2] < fanins2[2]) {
            return -1;
        } else if (fanins1[2] > fanins2[2]) {
            return 1;
        }

        return 0;
    }

    inline int colex_compare(const std::array<int, 3>& fanins1, const std::array<int, 3>& fanins2)
    {
        if (fanins1[2] < fanins2[2]) {
            return -1;
        } else if (fanins1[2] > fanins2[2]) {
            return 1;
        }
        if (fanins1[1] < fanins2[1]) {
            return -1;
        } else if (fanins1[1] > fanins2[1]) {
            return 1;
        }
        if (fanins1[0] < fanins2[0]) {
            return -1;
        } else if (fanins1[0] > fanins2[0]) {
            return 1;
        }

        return 0;
    }
}

