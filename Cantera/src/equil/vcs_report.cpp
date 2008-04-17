/*  
 * $Id$
 */

/*
 * Copywrite (2005) Sandia Corporation. Under the terms of 
 * Contract DE-AC04-94AL85000 with Sandia Corporation, the
 * U.S. Government retains certain rights in this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "vcs_solve.h"
#include "vcs_internal.h"
#include "vcs_VolPhase.h"

namespace VCSnonideal {

/*****************************************************************************/
static void print_space(int num)
{
 for (int j = 0; j < num; j++) {
   plogf(" ");
 }
}

static void print_line(std::string schar, int num) {
    for (int j = 0; j < num; j++) plogf("%s", schar.c_str());
    plogf("\n");
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int VCS_SOLVE::vcs_report(int iconv)
   
   /**************************************************************************
   *
   *  vcs_report:
   *
   *     Print out a report on the state of the equilibrium problem to 
   *     standard output.
   *     This prints out the current contents of the VCS_SOLVE class, V.
   ***************************************************************************/
{
   int i, j, l, k,  inertYes = FALSE, kspec;
   int nspecies = m_numSpeciesTot;
   double  g;

   char originalUnitsState = UnitsState;
   

   std::vector<int> sortindex(nspecies,0);
   std::vector<double> xy(nspecies,0.0);
   
   /* ************************************************************** */
   /* **** SORT DEPENDENT SPECIES IN DECREASING ORDER OF MOLES ***** */
   /* ************************************************************** */

   for (i = 0; i < nspecies; ++i) {
      sortindex[i] = i;
      xy[i] = soln[i];
   }
   /* 
   *       Sort the XY vector, the mole fraction vector, 
   *       and the sort index vector, sortindex, according to 
   *       the magnitude of the mole fraction vector. 
   */
   for (l = m_numComponents; l < m_numSpeciesRdc; ++l) {
      k = vcs_optMax(VCS_DATA_PTR(xy), 0, l, m_numSpeciesRdc);
      if (k != l) {
	 vcsUtil_dsw(VCS_DATA_PTR(xy), k, l);
	 vcsUtil_isw(VCS_DATA_PTR(sortindex), k, l);
      }
   }  
   
   /*
   *  Decide whether we have to nondimensionalize the equations.
   *     -> For the printouts from this routine, we will use nondimensional
   *        representations. This may be expanded in the future.
   */
   if (UnitsState == VCS_DIMENSIONAL_G) {
      vcs_nondim_TP();  
   }
   
   /* ******************************************************** */
   /* *** PRINT OUT RESULTS ********************************** */
   /* ******************************************************** */
   
   plogf("\n\n\n\n");
   print_line("-", 80);
   print_line("-", 80);
   plogf("\t\t VCS_TP REPORT\n");
   print_line("-", 80);
   print_line("-", 80);
   if (iconv < 0) {
      plogf(" ERROR: CONVERGENCE CRITERION NOT SATISFIED.\n");
   } else if (iconv == 1) {
     plogf(" RANGE SPACE ERROR: Equilibrium Found but not all Element Abundances are Satisfied\n");
   }
   /*
   *   Calculate some quantities that may need updating
   */
   vcs_tmoles();
   Vol = vcs_VolTotal(T, Pres, VCS_DATA_PTR(soln), VCS_DATA_PTR(VolPM));
   
   plogf("\t\tTemperature = %15.2g Kelvin\n", T);
   plogf("\t\tPressure    = %15.5g Atmos\n", Pres);
   plogf("\t\tVolume      = %15.5g cm**3\n", Vol);
   
   /* 
    * -------- TABLE OF SPECIES IN DECREASING MOLE NUMBERS --------------
    */
   plogf("\n\n");
   print_line("-", 80);
   plogf(" Species                 Equilibrium moles    ");
   plogf("Mole Fraction    ChemPot/RT    SpecUnkType\n");
   print_line("-", 80);
   for (i = 0; i < m_numComponents; ++i) {
      plogf(" %-12.12s", SpName[i].c_str());
      print_space(13);
      plogf("%14.7E     %14.7E    %12.4E", soln[i], wt[i], m_gibbsSpecies[i]);
      plogf("   %3d", SpeciesUnknownType[i]);
      plogf("\n");
   }
   for (i = m_numComponents; i < m_numSpeciesRdc; ++i) {
      l = sortindex[i];
      plogf(" %-12.12s", SpName[l].c_str());
      print_space(13);
     
      if (SpeciesUnknownType[l] == VCS_SPECIES_TYPE_MOLNUM) {
	plogf("%14.7E     %14.7E    %12.4E", soln[l], wt[l], m_gibbsSpecies[l]);
	plogf("   MolNum ");
      } else if (SpeciesUnknownType[l] == VCS_SPECIES_TYPE_INTERFACIALVOLTAGE) {
	plogf("        NA         %14.7E    %12.4E", 1.0, m_gibbsSpecies[l]);
	plogf("   Voltage = %14.7E", soln[l]);
      } else {
	plogf("we have a problem\n");
	exit(-1);
      }
      plogf("\n");
   }
   for (i = 0; i < NPhase; i++) {
      if (TPhInertMoles[i] > 0.0) {
	 inertYes = TRUE;
	 if (i == 0) {
	    plogf(" Inert Gas Species        ");
	 } else {
	    plogf(" Inert Species in phase %16s ", 
		   (VPhaseList[i])->PhaseName.c_str());
	 }
	 plogf("%14.7E     %14.7E    %12.4E\n", TPhInertMoles[i], 
		TPhInertMoles[i] /  TPhMoles[i], 0.0); 
      }
   }
   if (m_numSpeciesRdc != nspecies) {
      plogf("\n SPECIES WITH LESS THAN 1.0E-32 MOLES:\n\n");
      for (kspec = m_numSpeciesRdc; kspec < nspecies; ++kspec) {
	 plogf(" %-12.12s", SpName[kspec].c_str());
	 plogf("             %14.7E     %14.7E    %12.4E",
		soln[kspec], wt[kspec], dg[kspec]);
	 if (SpeciesUnknownType[i] == VCS_SPECIES_TYPE_MOLNUM) {
	   plogf("   Mol_Num");
	 } else if (SpeciesUnknownType[i] == VCS_SPECIES_TYPE_INTERFACIALVOLTAGE) {
	   plogf("   Voltage");
	 } else {
	   plogf("   Unknown");
	 }

	 plogf("\n");
      }
   }
   print_line("-", 80);
   plogf("\n");

   /* 
    * ---------- TABLE OF SPECIES FORMATION REACTIONS ------------------
    */
   plogf("\n");
   print_line("-", m_numComponents*10 + 45);
   plogf("               |ComponentID|");
   for (j = 0; j < m_numComponents; j++) {
     plogf("        %3d", j);
   }
   plogf(" |           |\n");
   plogf("               | Components|");
   for (j = 0; j < m_numComponents; j++) {
     plogf(" %10.10s", SpName[j].c_str());
   }
   plogf(" |           |\n");
   plogf(" NonComponent  |   Moles   |");
   for (j = 0; j < m_numComponents; j++) {
     plogf(" %10.3g", soln[j]);
   }
   plogf(" | DG/RT Rxn |\n");
   print_line("-", m_numComponents*10 + 45);
   for (i = 0; i < m_numRxnTot; i++) {
     int irxn = ir[i];
     plogf(" %3d ", irxn);
     plogf("%-10.10s", SpName[irxn].c_str());
	plogf("|%10.3g |", soln[irxn]);
	for (j = 0; j < m_numComponents; j++) {
	  plogf("     %6.2f", sc[i][j]);
	}
	plogf(" |%10.3g |", dg[irxn]);
	plogf("\n");
   }
   print_line("-", m_numComponents*10 + 45);
   plogf("\n");

   /* 
    * ------------------ TABLE OF PHASE INFORMATION ---------------------
    */
   std::vector<double> gaPhase(m_numElemConstraints, 0.0);
   std::vector<double> gaTPhase(m_numElemConstraints, 0.0);
   double totalMoles = 0.0;
   double gibbsPhase = 0.0;
   double gibbsTotal = 0.0;
   plogf("\n\n");
   plogf("\n");
   print_line("-", m_numElemConstraints*10 + 58);
   plogf("                  | ElementID |");
   for (j = 0; j < m_numElemConstraints; j++) {
     plogf("        %3d", j);
   }
   plogf(" |                     |\n");
   plogf("                  | Element   |");
   for (j = 0; j < m_numElemConstraints; j++) {
     plogf(" %10.10s", (ElName[j]).c_str());
   }
   plogf(" |                     |\n");
   plogf("    PhaseName     | MolTarget |");
   for (j = 0; j < m_numElemConstraints; j++) {
     plogf(" %10.3g", gai[j]);
   }
   plogf(" |     Gibbs Total     |\n");
   print_line("-", m_numElemConstraints*10 + 58);
   for (int iphase = 0; iphase < NPhase; iphase++) {
     plogf(" %3d ", iphase);
     vcs_VolPhase *VPhase = VPhaseList[iphase];
     plogf("%-12.12s |",VPhase->PhaseName.c_str());
     plogf("%10.3e |", TPhMoles[iphase]);
     totalMoles +=  TPhMoles[iphase];
     if (TPhMoles[iphase] != VPhase->TotalMoles()) {
       if (! vcs_doubleEqual(TPhMoles[iphase], VPhase->TotalMoles())) {
	 plogf("We have a problem\n");
	 exit(-1);
       }
     }
     vcs_elabPhase(iphase, VCS_DATA_PTR(gaPhase));
     for (j = 0; j < m_numElemConstraints; j++) {
       plogf(" %10.3g", gaPhase[j]);
       gaTPhase[j] += gaPhase[j];
     }
     gibbsPhase = vcs_GibbsPhase(iphase, VCS_DATA_PTR(soln), 
				 VCS_DATA_PTR(m_gibbsSpecies));
     gibbsTotal += gibbsPhase;
     plogf(" | %18.11E |\n", gibbsPhase);
   }
   print_line("-", m_numElemConstraints*10 + 58);
   plogf("    TOTAL         |%10.3e |", totalMoles);
   for (j = 0; j < m_numElemConstraints; j++) {
       plogf(" %10.3g", gaTPhase[j]);
   }
   plogf(" | %18.11E |\n", gibbsTotal);

   print_line("-", m_numElemConstraints*10 + 58);
   plogf("\n");

   /*
    * ----------- GLOBAL SATISFACTION INFORMATION -----------------------
    */

   /*
    *    Calculate the total dimensionless Gibbs Free Energy
    *     -> Inert species are handled as if they had a standard free
    *        energy of zero
    */
	  
   g = vcs_Total_Gibbs(VCS_DATA_PTR(soln), VCS_DATA_PTR(m_gibbsSpecies), 
		       VCS_DATA_PTR(TPhMoles));
   plogf("\n\tTotal Dimensionless Gibbs Free Energy = G/RT = %15.7E\n", g);
   if (inertYes) 
      plogf("\t\t(Inert species have standard free energy of zero)\n");
   
   plogf("\nElemental Abundances:     ");
   plogf("         Actual                    Target         Type      ElActive\n");
   for (i = 0; i < m_numElemConstraints; ++i) {
      print_space(26); plogf("%-2.2s", (ElName[i]).c_str());
      plogf("%20.12E  %20.12E", ga[i], gai[i]);
      plogf("   %3d     %3d\n", m_elType[i], ElActive[i]);
   }
   plogf("\n");
   
   /* 
    * ------------------ TABLE OF SPECIES CHEM POTS ---------------------
    */
   plogf("\n"); print_line("-", 93);
   plogf("Chemical Potentials of the Species: (dimensionless)\n");

   double rt = vcs_nondimMult_TP(m_VCS_UnitsFormat, T);
   plogf("\t\t(RT = %g ", rt);
   vcs_printChemPotUnits(m_VCS_UnitsFormat);
   plogf(")\n");
   plogf("    Name         TMoles     StandStateChemPot   "
	  "   ln(AC)       ln(X_i)      |   F z_i phi   |    ChemPot    | (-lnMnaught)\n");
   print_line("-", 115);
   for (i = 0; i < nspecies; ++i) {
      l = sortindex[i];
      int pid = PhaseID[l];
      plogf(" %-12.12s", SpName[l].c_str());
      plogf(" %14.7E ", soln[l]);
      plogf("%14.7E  ", m_SSfeSpecies[l]);
      plogf("%14.7E  ", log(ActCoeff[l]));
      double tpmoles = TPhMoles[pid];
      double phi = phasePhi[pid];
      double eContrib = phi * Charge[l] * Faraday_dim;
      double lx = 0.0;
      if (SpeciesUnknownType[l] == VCS_SPECIES_TYPE_INTERFACIALVOLTAGE) {
	lx = 0.0;
      } else {
	if (tpmoles > 0.0 && soln[l] > 0.0) {
	  lx = log(soln[l]) - log(tpmoles);
	} else {
	  lx = m_gibbsSpecies[l] - m_SSfeSpecies[l] - log(ActCoeff[l]) + SpecLnMnaught[l];
	}
      }
      plogf("%14.7E  |", lx);
      plogf("%14.7E | ", eContrib);
      double tmp = m_SSfeSpecies[l] + log(ActCoeff[l]) + lx - SpecLnMnaught[l] + eContrib;
      if (fabs(m_gibbsSpecies[l] - tmp) > 1.0E-8) {
	plogf("\n\t\twe have a problem - doesn't add up\n");
	exit(-1);
      } 
      plogf(" %12.4E |", m_gibbsSpecies[l]);
      if (SpecLnMnaught[l] != 0.0) {
	plogf(" (%14.7E)", - SpecLnMnaught[l]);
      }
      plogf("\n");
   }
   print_line("-", 115);

   /*
    * ------------- TABLE OF SOLUTION COUNTERS --------------------------
    */
   plogf("\n");
   plogf("\nCounters:         Iterations          Time (seconds)\n");
   if (m_timing_print_lvl > 0) {
     plogf("    vcs_basopt:   %5d             %11.5E\n",
	   m_VCount->Basis_Opts, m_VCount->Time_basopt);
     plogf("    vcs_TP:       %5d             %11.5E\n", 
	   m_VCount->Its, m_VCount->Time_vcs_TP);
   } else {
     plogf("    vcs_basopt:   %5d             %11s\n",
	   m_VCount->Basis_Opts,"    NA     ");
     plogf("    vcs_TP:       %5d             %11s\n", 
	   m_VCount->Its,"    NA     " );
   }
   print_line("-", 80);
   print_line("-", 80);
   
   /*
   *   Set the Units state of the system back to where it was when we
   *   entered the program.
   */
   if (originalUnitsState != UnitsState) {
      if (originalUnitsState == VCS_DIMENSIONAL_G ) vcs_redim_TP();
      else                                          vcs_nondim_TP();
   }
   /*
   *   Return a successful completion flag
   */
   return VCS_SUCCESS;
} /* vcs_report() ************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void VCS_SOLVE::vcs_TCounters_report(int timing_print_lvl)
   
   /**************************************************************************
   *
   * vcs_TCounters_report:
   *
   *   Print out the total Its and time counters to standard output
   ***************************************************************************/
{
  plogf("\nTCounters:   Num_Calls   Total_Its       Total_Time (seconds)\n");
  if (timing_print_lvl > 0) {
    plogf("    vcs_basopt:   %5d      %5d         %11.5E\n",
	  m_VCount->T_Basis_Opts, m_VCount->T_Basis_Opts,
	  m_VCount->T_Time_basopt);
    plogf("    vcs_TP:       %5d      %5d         %11.5E\n", 
	  m_VCount->T_Calls_vcs_TP, m_VCount->T_Its, 
	  m_VCount->T_Time_vcs_TP);
    plogf("    vcs_inest:    %5d                    %11.5E\n", 
	  m_VCount->T_Calls_Inest,  m_VCount->T_Time_inest);
    plogf("    vcs_TotalTime:                         %11.5E\n",
	  m_VCount->T_Time_vcs);
  } else {
    plogf("    vcs_basopt:   %5d      %5d         %11s\n",
	  m_VCount->T_Basis_Opts, m_VCount->T_Basis_Opts,"    NA     ");
    plogf("    vcs_TP:       %5d      %5d         %11s\n", 
	  m_VCount->T_Calls_vcs_TP, m_VCount->T_Its,"    NA     ");
    plogf("    vcs_inest:    %5d                    %11s\n", 
	  m_VCount->T_Calls_Inest, "    NA     ");
    plogf("    vcs_TotalTime:                         %11s\n",
	  "    NA     ");
  }
}
   
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
}
 
