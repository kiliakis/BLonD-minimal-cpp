#include <iostream>

#include <gtest/gtest.h>
#include <blond/math_functions.h>
#include <blond/utilities.h>
#include <blond/beams/Distributions.h>

using namespace blond;

const ftype epsilon = 1e-8;
const std::string track_params = "../unit-tests/references/Slices/Slices_track_params/";
const std::string set_cuts_params = "../unit-tests/references/Slices/Slices_set_cuts_params/";
const std::string sort_particles_params = "../unit-tests/references/Slices/Slices_sort_particles_params/";


class testSlices : public ::testing::Test {

protected:
   const long N_b = 1e9;           // Intensity
   const ftype tau_0 = 0.4e-9;          // Initial bunch length, 4 sigma [s]

   virtual void SetUp()
   {
      ftype *momentum = new ftype[N_t + 1];
      mymath::linspace(momentum, p_i, p_f, N_t + 1);

      ftype *alpha_array = new ftype[(alpha_order + 1) * n_sections];

      std::fill_n(alpha_array, (alpha_order + 1) * n_sections, alpha);

      ftype *C_array = new ftype[n_sections];
      std::fill_n(C_array, n_sections, C);

      ftype *h_array = new ftype[n_sections * (N_t + 1)];
      std::fill_n(h_array, (N_t + 1) * n_sections, h);

      ftype *V_array = new ftype[n_sections * (N_t + 1)];
      std::fill_n(V_array, (N_t + 1) * n_sections, V);

      ftype *dphi_array = new ftype[n_sections * (N_t + 1)];
      std::fill_n(dphi_array, (N_t + 1) * n_sections, dphi);

      context.GP = new GeneralParameters(N_t, C_array, alpha_array, alpha_order, momentum,
                                 proton);

      context.Beam = new Beams(N_p, N_b);

      context.RfP = new RfParameters(n_sections, h_array, V_array, dphi_array);

      longitudinal_bigaussian(tau_0 / 4, 0, 1, false);

      context.Slice = new Slices(N_slices);

   }


   virtual void TearDown()
   {
      // Code here will be called immediately after each test
      // (right before the destructor).
      delete context.GP;
      delete context.Beam;
      delete context.RfP;
      delete context.Slice;
   }


private:

   // Machine and RF parameters
   const ftype C = 26658.883;          // Machine circumference [m]
   const long p_i = 450e9;          // Synchronous momentum [eV/c]
   const ftype p_f = 460.005e9;          // Synchronous momentum, final
   const long h = 35640;          // Harmonic number
   const ftype V = 6e6;          // RF voltage [V]
   const ftype dphi = 0;          // Phase modulation/offset
   const ftype gamma_t = 55.759505;          // Transition gamma
   const ftype alpha = 1.0 / gamma_t / gamma_t; // First order mom. comp. factor
   const int alpha_order = 1;
   const int n_sections = 1;
   // Tracking details

   const int N_t = 2000;    // Number of turns to track
   const int N_p = 100;         // Macro-particles
   const int N_slices = 10;


};



TEST_F(testSlices, set_cuts_left)
{
   //putenv("FIXED_PARTICLES=1");

   std::vector<ftype> v;

   util::read_vector_from_file(v, set_cuts_params + "cut_left");
   ftype ref = v[0];
   ftype real = context.Slice->cut_left;
   ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
}

TEST_F(testSlices, set_cuts_right)
{
   //putenv("FIXED_PARTICLES=1");

   std::vector<ftype> v;

   util::read_vector_from_file(v, set_cuts_params + "cut_right");
   ftype ref = v[0];
   ftype real = context.Slice->cut_right;
   ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
}

TEST_F(testSlices, set_cuts_bin_centers)
{
   //putenv("FIXED_PARTICLES=1");

   std::vector<ftype> v;

   util::read_vector_from_file(v, set_cuts_params + "bin_centers");
   for (unsigned int i = 0; i < v.size(); ++i) {
      ftype ref = v[i];
      ftype real = context.Slice->bin_centers[i];
      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
   }
}

// TODO test set_cuts a little further


TEST_F(testSlices, sort_particles_dE)
{

   std::vector<ftype> v;
   util::read_vector_from_file(v, sort_particles_params + "dE");
   for (unsigned int i = 0; i < v.size(); ++i) {
      ftype ref = v[i];
      ftype real = context.Beam->dE[i];
      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
   }
}

TEST_F(testSlices, sort_particles_dt)
{

   std::vector<ftype> v;
   util::read_vector_from_file(v, sort_particles_params + "dt");
   for (unsigned int i = 0; i < v.size(); ++i) {
      ftype ref = v[i];
      ftype real = context.Beam->dt[i];
      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
   }
}

TEST_F(testSlices, sort_particles_id)
{

   std::vector<ftype> v;
   util::read_vector_from_file(v, sort_particles_params + "id");
   for (unsigned int i = 0; i < v.size(); ++i) {
      ftype ref = v[i];
      ftype real = context.Beam->id[i];
      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
   }
}

TEST_F(testSlices, n_macroparticles)
{

   //RingAndRfSection *long_tracker = new RingAndRfSection();
   //long_tracker->track(0, Beam->n_macroparticles);
   auto Slice = context.Slice;
   Slice->track();
   //util::dump(Slice->n_macroparticles, 100, "something\n");
   std::vector<ftype> v;
   util::read_vector_from_file(v, track_params + "n_macroparticles");
   for (unsigned int i = 0; i < v.size(); ++i) {
      ftype ref = v[i];
      ftype real = Slice->n_macroparticles[i];
      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));
   }
}



int main(int ac, char *av[])
{
   ::testing::InitGoogleTest(&ac, av);
   return RUN_ALL_TESTS();
}