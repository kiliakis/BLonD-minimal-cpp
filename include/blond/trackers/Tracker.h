/*
 * Tracker.h
 *
 *  Created on: Mar 10, 2016
 *      Author: kiliakis
 */

#ifndef TRACKERS_TRACKER_H_
#define TRACKERS_TRACKER_H_

#include <blond/utilities.h>
#include <blond/llrf/PhaseLoop.h>
namespace blond {

	enum solver_type {
		simple, full
	};

	// !!!!WARNING!!!!
	// we only use beam->dt, dE
	// bool arrays are used every time to update only the right values!!

	class API RingAndRfSection {

	private:
	public:
		ftype elapsed_time;

		bool *indices_right_outside;
		bool *indices_inside_frame;
		bool *indices_left_outside;
		inline void set_periodicity(const int start, const int end);
		inline void kick(const bool *update, const int index, const int start,
			const int end);
		inline void kick(const ftype *__restrict beam_dt,
			ftype *__restrict beam_dE, const int n_rf,
			const ftype *__restrict voltage,
			const ftype *__restrict omega_RF,
			const ftype *__restrict phi_RF, const int n_macroparticles,
			const ftype acc_kick, const bool *__restrict update,
			const int start, const int end);
		inline void kick(const int index, const int start, const int end);
		inline void kick(const ftype *__restrict beam_dt,
			ftype *__restrict beam_dE, const int n_rf,
			const ftype *__restrict voltage,
			const ftype *__restrict omega_RF,
			const ftype *__restrict phi_RF, const int n_macroparticles,
			const ftype acc_kick, const int start, const int end);
		inline void drift(const bool *update, const int index, const int start,
			const int end);
		inline void drift(ftype *__restrict beam_dt,
			const ftype *__restrict beam_dE, const solver_type solver,
			const ftype T0, const ftype length_ratio, const int alpha_order,
			const ftype eta_zero, const ftype eta_one, const ftype eta_two,
			const ftype beta, const ftype energy, const int n_macroparticles,
			const bool *__restrict update, const int start, const int end);
		inline void drift(const int index, const int start, const int end);
		inline void drift(ftype *__restrict beam_dt,
			const ftype *__restrict beam_dE, const solver_type solver,
			const ftype T0, const ftype length_ratio, const int alpha_order,
			const ftype eta_zero, const ftype eta_one, const ftype eta_two,
			const ftype beta, const ftype energy, const int n_macroparticles,
			const int start, const int end);

		void track(const int start, const int end);
		inline void horizontal_cut(const int start, const int end);
		RingAndRfSection(solver_type solver = simple, PhaseLoop *PL = NULL,
			ftype *NoiseFB = NULL, bool periodicity = false, ftype dE_max = 0,
			bool rf_kick_interp = false, ftype *Slices = NULL,
			ftype *TotalInducedVoltage = NULL);
		~RingAndRfSection();

		solver_type solver;
		PhaseLoop *PL;
		ftype *NoiseFB;
		bool periodicity;
		ftype dE_max;
		bool rf_kick_interp;
		ftype *Slices;
		ftype *TotalInducedVoltage;
		//int n_threads;
		ftype *acceleration_kick;

	};
}
#endif /* TRACKERS_TRACKER_H_ */
