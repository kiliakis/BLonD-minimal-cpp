
#include <blond/beams/Beams.h>
#include <blond/beams/Distributions.h>
#include <blond/beams/Slices.h>
#include <blond/globals.h>
#include <blond/impedances/InducedVoltage.h>
#include <blond/input_parameters/GeneralParameters.h>
#include <blond/input_parameters/RfParameters.h>
#include <blond/math_functions.h>
#include <blond/trackers/Tracker.h>
#include <blond/utilities.h>
#include <gtest/gtest.h>
#include <stdio.h>

using namespace std;

class testInducedVoltage : public ::testing::Test {

protected:
    // Simulation parameters
    // --------------------------------------------------------
    // Bunch parameters
    const long long N_b = 1e10; // Intensity
    const ftype tau_0 = 2e-9; // Initial bunch length, 4 sigma [s]
    // const particle_type particle = proton;
    // Machine and RF parameters
    const ftype C = 6911.56;   // Machine circumference [m]
    const ftype p_i = 25.92e9; // Synchronous momentum [eV/c]
    // const ftype p_f = 460.005e9;                  // Synchronous momentum,
    // final
    const long h = 4620;                     // Harmonic number
    const ftype V = 0.9e6;                        // RF voltage [V]
    const ftype dphi = 0;                         // Phase modulation/offset
    const ftype gamma_t = 1 / std::sqrt(0.00192); // Transition gamma
    const ftype alpha =
        1.0 / gamma_t / gamma_t; // First order mom. comp. factor
    const int alpha_order = 1;
    const int n_sections = 1;
    // Tracking details

    long N_t = 1000;       // Number of turns to track
    long N_p = 5000; // Macro-particles

    long N_slices = 1 << 8; // = (2^8)
    const string datafiles = DEMO_FILES "/TC5_Wake_impedance/";
    Resonators *resonator;

    virtual void SetUp()
    {

        omp_set_num_threads(1);

        f_vector_2d_t momentumVec(n_sections, f_vector_t(N_t + 1, p_i));

        f_vector_2d_t alphaVec(n_sections, f_vector_t(alpha_order + 1, alpha));

        f_vector_t CVec(n_sections, C);

        f_vector_2d_t hVec(n_sections, f_vector_t(N_t + 1, h));

        f_vector_2d_t voltageVec(n_sections, f_vector_t(N_t + 1, V));

        f_vector_2d_t dphiVec(n_sections, f_vector_t(N_t + 1, dphi));

        Context::GP = new GeneralParameters(N_t, CVec, alphaVec, alpha_order,
                                            momentumVec, proton);

        Context::Beam = new Beams(N_p, N_b);

        Context::RfP = new RfParameters(n_sections, hVec, voltageVec, dphiVec);


        longitudinal_bigaussian(tau_0 / 4, 0, -1, false);

        Context::Slice = new Slices(N_slices, 0, 0, 2 * constant::pi, rad);

        f_vector_t v;
        util::read_vector_from_file(v, datafiles + "TC5_new_HQ_table.dat");
        assert(v.size() % 3 == 0);

        f_vector_t R_shunt, f_res, Q_factor;

        R_shunt.reserve(v.size() / 3);
        f_res.reserve(v.size() / 3);
        Q_factor.reserve(v.size() / 3);

        for (uint i = 0; i < v.size(); i += 3) {
            f_res.push_back(v[i] * 1e9);
            Q_factor.push_back(v[i + 1]);
            R_shunt.push_back(v[i + 2] * 1e6);
        }

        resonator = new Resonators(R_shunt, f_res, Q_factor);
    }

    virtual void TearDown()
    {
        // Code here will be called immediately after each test
        // (right before the destructor).
        delete Context::GP;
        delete Context::Beam;
        delete Context::RfP;
        delete Context::Slice;
        delete resonator;
    }
};


class testTotalInducedVoltage : public testInducedVoltage {};


TEST_F(testInducedVoltage, constructor1)
{
    ftype epsilon = 1e-8;
    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);

    auto params = std::string(TEST_FILES "/Impedances/") +
                  "InducedVoltage/InducedVoltageTime/constructor1/";

    f_vector_t v;
    util::read_vector_from_file(v, params + "time_array.txt");

    ASSERT_EQ(v.size(), indVoltTime->fTimeArray.size());

    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = indVoltTime->fTimeArray[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of indVoltTime->fTimeArray failed on i " << i
                << std::endl;
    }

    util::read_vector_from_file(v, params + "total_wake.txt");
    ASSERT_EQ(v.size(), indVoltTime->fTotalWake.size());

    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = indVoltTime->fTotalWake[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of indVoltTime->fTotalWake failed on i " << i
                << std::endl;
    }

    util::read_vector_from_file(v, params + "cut.txt");
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], indVoltTime->fCut) << "Testing of fCut failed\n";

    util::read_vector_from_file(v, params + "fshape.txt");
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], indVoltTime->fShape) << "Testing of fShape failed\n";

    delete indVoltTime;
}


TEST_F(testInducedVoltage, reprocess1)
{

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    auto Slice = Context::Slice;
    Slice->track();

    for (uint i = 0; i < Slice->n_slices; i++)
        Slice->bin_centers[i] *= 1.1;

    indVoltTime->reprocess();

    std::string params = std::string(TEST_FILES "/Impedances/") +
                         "InducedVoltage/InducedVoltageTime/reprocess1/";

    f_vector_t v;
    util::read_vector_from_file(v, params + "time_array.txt");

    ASSERT_EQ(v.size(), indVoltTime->fTimeArray.size());

    ftype epsilon = 1e-8;

    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = indVoltTime->fTimeArray[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of indVoltTime->fTimeArray failed on i " << i
                << std::endl;
    }

    util::read_vector_from_file(v, params + "total_wake.txt");
    ASSERT_EQ(v.size(), indVoltTime->fTotalWake.size());

    epsilon = 1e-8;
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = indVoltTime->fTotalWake[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of indVoltTime->fTotalWake failed on i " << i
                << std::endl;
    }

    util::read_vector_from_file(v, params + "cut.txt");

    for (uint i = 0; i < v.size(); ++i) {
        uint ref = v[i];
        uint real = indVoltTime->fCut;
        ASSERT_EQ(ref, real) << "Testing of indVoltTime->fCut failed on i " << i
                             << std::endl;
    }

    util::read_vector_from_file(v, params + "fshape.txt");

    for (uint i = 0; i < v.size(); ++i) {
        uint ref = v[i];
        uint real = indVoltTime->fShape;
        ASSERT_EQ(ref, real) << "Testing of fShape failed on i " << i
                             << std::endl;
    }

    delete indVoltTime;
}

TEST_F(testInducedVoltage, generation1)
{
    Context::Slice->track();
    auto epsilon = 1e-8;


    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    f_vector_t res = indVoltTime->induced_voltage_generation();

    std::string params = std::string(TEST_FILES "/Impedances/") +
                         "InducedVoltage/InducedVoltageTime/generation1/";

    f_vector_t v;
    util::read_vector_from_file(v, params + "induced_voltage.txt");

    ASSERT_EQ(v.size(), res.size());

    ftype max = *max_element(res.begin(), res.end(), [](ftype i, ftype j) {
        return std::abs(i) < std::abs(j);
    });
    max = std::abs(max);
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];

        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of indVoltTime->fInducedVoltage failed on i " << i
                << std::endl;
    }

    delete indVoltTime;
}


TEST_F(testInducedVoltage, generation2)
{
    Context::Slice->track();
    auto epsilon = 1e-7;

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    f_vector_t res = indVoltTime->induced_voltage_generation(100);

    std::string params = std::string(TEST_FILES "/Impedances/") +
                         "InducedVoltage/InducedVoltageTime/generation2/";

    f_vector_t v;
    util::read_vector_from_file(v, params + "induced_voltage.txt");

    ASSERT_EQ(v.size(), res.size());

    ftype max = *max_element(res.begin(), res.end(), [](ftype i, ftype j) {
        return std::abs(i) < std::abs(j);
    });
    max = std::abs(max);

    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];
        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of indVoltTime->fInducedVoltage failed on i " << i
                << std::endl;
    }

    delete indVoltTime;
}


TEST_F(testInducedVoltage, convolution1)
{
    Context::Slice->track();
    auto epsilon = 1e-8;

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime =
        new InducedVoltageTime(wakeSourceList, time_or_freq::time_domain);
    f_vector_t res = indVoltTime->induced_voltage_generation();

    std::string params = std::string(TEST_FILES "/Impedances/") +
                         "InducedVoltage/InducedVoltageTime/convolution1/";

    f_vector_t v;
    util::read_vector_from_file(v, params + "induced_voltage.txt");

    ASSERT_EQ(v.size(), res.size());

    ftype max = *max_element(res.begin(), res.end(), [](ftype i, ftype j) {
        return std::abs(i) < std::abs(j);
    });
    max = std::abs(max);

    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];
        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of indVoltTime->fInducedVoltage failed on i " << i
                << std::endl;
    }
    delete indVoltTime;
}



TEST_F(testInducedVoltage, track1)
{
    std::string params = std::string(TEST_FILES "/Impedances/") +
                         "InducedVoltage/InducedVoltageTime/track1/";
    auto epsilon = 1e-8;

    auto Beam = Context::Beam;
    Context::Slice->track();

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    indVoltTime->track();

    f_vector_t v;
    util::read_vector_from_file(v, params + "dE.txt");
    ASSERT_EQ(v.size(), Beam->dE.size());

    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = Beam->dE[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of Beam->dE failed on i " << i << std::endl;
    }

    delete indVoltTime;
}


TEST_F(testTotalInducedVoltage, sum1)
{

    auto epsilon = 1e-8;
    auto params = std::string(TEST_FILES "/Impedances/") +
                  "InducedVoltage/TotalInducedVoltage/sum1/";

    Context::Slice->track();

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    std::vector<InducedVoltage *> indVoltList({indVoltTime});
    auto totVol = new TotalInducedVoltage(indVoltList);

    f_vector_t res = totVol->induced_voltage_sum(200);

    f_vector_t v;
    util::read_vector_from_file(v, params + "extIndVolt.txt");
    ASSERT_EQ(v.size(), res.size());

    ftype max = *max_element(res.begin(), res.end(), [](ftype i, ftype j) {
        return std::abs(i) < std::abs(j);
    });
    max = std::abs(max);
    // warning checking only the first 100 elems
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];

        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of extIndVolt failed on i "
                << i << std::endl;
    }

    res.clear();

    res = totVol->fInducedVoltage;
    util::read_vector_from_file(v, params + "induced_voltage.txt");
    ASSERT_EQ(v.size(), res.size());

    max = *max_element(res.begin(), res.end(),
    [](ftype i, ftype j) { return std::abs(i) < std::abs(j); });
    max = std::abs(max);
    // warning checking only the first 100 elems
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];

        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of fInducedVoltage failed on i " << i << std::endl;
    }

    delete indVoltTime;
    delete totVol;
}


TEST_F(testTotalInducedVoltage, sum2)
{

    auto epsilon = 1e-8;
    auto params = std::string(TEST_FILES "/Impedances/") +
                  "InducedVoltage/TotalInducedVoltage/sum2/";

    Context::Slice->track();

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime1 = new InducedVoltageTime(wakeSourceList);
    auto indVoltTime2 = new InducedVoltageTime(wakeSourceList);
    std::vector<InducedVoltage *> indVoltList({indVoltTime1, indVoltTime2});
    auto totVol = new TotalInducedVoltage(indVoltList);

    f_vector_t res = totVol->induced_voltage_sum(200);

    f_vector_t v;
    util::read_vector_from_file(v, params + "extIndVolt.txt");
    ASSERT_EQ(v.size(), res.size());

    ftype max = *max_element(res.begin(), res.end(), [](ftype i, ftype j) {
        return std::abs(i) < std::abs(j);
    });
    max = std::abs(max);
    // warning checking only the first 100 elems
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];

        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of extIndVolt failed on i "
                << i << std::endl;
    }

    res.clear();

    res = totVol->fInducedVoltage;
    util::read_vector_from_file(v, params + "induced_voltage.txt");
    ASSERT_EQ(v.size(), res.size());

    max = *max_element(res.begin(), res.end(),
    [](ftype i, ftype j) { return std::abs(i) < std::abs(j); });
    max = std::abs(max);
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = res[i];

        ASSERT_NEAR(ref, real, epsilon * max)
                << "Testing of fInducedVoltage failed on i " << i << std::endl;
    }

    delete indVoltTime1;
    delete indVoltTime2;
    delete totVol;
}





TEST_F(testTotalInducedVoltage, track1)
{
    auto epsilon = 1e-8;
    auto params = std::string(TEST_FILES "/Impedances/") +
                  "InducedVoltage/TotalInducedVoltage/track1/";

    auto Beam = Context::Beam;

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    std::vector<InducedVoltage *> indVoltList({indVoltTime});

    auto totVol = new TotalInducedVoltage(indVoltList);

    for (int i = 0; i < 1000; i++) totVol->track();

    f_vector_t v;
    util::read_vector_from_file(v, params + "dE.txt");
    ASSERT_EQ(v.size(), Beam->dE.size());
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = Beam->dE[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of Beam->dE failed on i " << i << std::endl;
    }

    delete indVoltTime;
    delete totVol;
}


TEST_F(testTotalInducedVoltage, track2)
{
    auto epsilon = 1e-8;
    auto params = std::string(TEST_FILES "/Impedances/") +
                  "InducedVoltage/TotalInducedVoltage/track2/";

    auto Beam = Context::Beam;

    std::vector<Intensity *> wakeSourceList({resonator});
    auto indVoltTime = new InducedVoltageTime(wakeSourceList);
    auto indVoltTime2 = new InducedVoltageTime(wakeSourceList);
    std::vector<InducedVoltage *> indVoltList({indVoltTime, indVoltTime2});

    auto totVol = new TotalInducedVoltage(indVoltList);

    for (int i = 0; i < 100; i++) totVol->track();

    f_vector_t v;
    util::read_vector_from_file(v, params + "dE.txt");
    ASSERT_EQ(v.size(), Beam->dE.size());
    for (uint i = 0; i < v.size(); ++i) {
        ftype ref = v[i];
        ftype real = Beam->dE[i];
        ASSERT_NEAR(ref, real, epsilon * std::max(std::abs(ref), std::abs(real)))
                << "Testing of Beam->dE failed on i " << i << std::endl;
    }

    delete indVoltTime;
    delete indVoltTime2;
    delete totVol;
}


int main(int ac, char *av[])
{
    ::testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
