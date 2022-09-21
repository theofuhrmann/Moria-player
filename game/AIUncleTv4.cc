#include "Player.hh"


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME UncleTv4

/* TO DO LIST:
 * Make Dwarfs hunt treasures and kill enemies if needed
 * Make Wizards adjacent to as many Dwarfs as possible
 * Avoid the balrog
 * 2-cell radius function
 */

struct PLAYER_NAME : public Player {

  	/**
   	* Factory: returns a new instance of this class.
   	* Do not modify this function.
   	*/
	static Player* factory () {
   		return new PLAYER_NAME;
  	}

  	/**
   	* Types and attributes for your player can be defined here.
   	*/

	typedef vector<int> V;
	typedef vector< vector<int> > Mat;
	#define infinity 2000000

	// Sets all weights and finds all the treasures in the graph
	// W: weigheted matrix, T: vector containing all the treasures' positions
	void dwarf_weights_and_treasures(Mat& W, vector<Pos>& T) {

		// Sets weights:
		// - Cave or Outside -> 0
		// - Rock -> number of hits left [1,5]
		// - Abyss, Granite -> infinity
		// - Troll cell + 1-cell radius -> infinity
		// - Balrog cell + 2-cell radius -> infinity
		// - Ally-Occupied cell -> infinity
		for (int i = 0; i < W.size(); i++)
			for (int j = 0; j < W[0].size(); j++) {
				if (cell(i,j).id != -1 and unit(cell(i,j).id).player == me())
					W[i][j] = infinity;
				else if (cell(i,j).type == Cave or cell(i,j).type == Outside) {
					if (cell(i,j).treasure) T.push_back(Pos(i,j));
					W[i][j] = 0;
				}
				else if (cell(i,j).type == Rock) W[i][j] = cell(i,j).turns;
				else W[i][j] = infinity;
			}

		Pos pos_balrog = unit(balrog_id()).pos;
		for (int i = 0; i < 8; i++) {
			Pos aux = pos_balrog + Dir(i);
			if (pos_ok(aux)) {
				W[aux.i][aux.j] = infinity;
				if (i%2 == 0) {
					Pos aux2 = aux + Dir(i);
					if (pos_ok(aux2)) W[aux2.i][aux2.j] = infinity;
				}
				else {
					for (int j = i-1; j <= i+1; j++) {
						Pos aux2 = aux + Dir(j%8);
						if (pos_ok(aux2)) W[aux2.i][aux2.j] = infinity;
					}
				}
			}
		}
		vector<int> VT = trolls();
		for (int i = 0; i < VT.size(); i++)
			for (int j = 0; j < 8; j++) {
				Pos p = unit(VT[i]).pos + Dir(j);
				if (pos_ok(p)) W[p.i][p.j] = infinity;
			}
	}

	// Sets all weights for wizards;
	// W: weigheted matrix
	void wizard_weights(Mat& W) {

		// Sets weights:
		// - Cave or Outside -> 0
		// - Abyss, Granite or Rock -> infinity
		// - Enemy cell + 1-cell radius -> infinity
		// - Balrog cell + 2-cell radius -> infinity
		// - Ally Dwarf 1-cell cross -> 0 (main destination)
		// - Occupied cell -> infinity
		for (int i = 0; i < W.size(); i++)
			for (int j = 0; j < W[0].size(); j++) {
				if (cell(i,j).id != -1) {
					if (cell(i,j).type == Cave or cell(i,j).type == Outside)
						W[i][j] = 0;
				}
				else {
					W[i][j] = infinity;
					if (unit(cell(i,j).id).player != me())
						for (int k = 0; k < 8; k++) {
							Pos aux = Pos(i,j) + Dir(i);
							if (pos_ok(aux)) W[aux.i][aux.j] = infinity;
						}
			   }
			}
		Pos pos_balrog = unit(balrog_id()).pos;
		for (int i = 0; i < 8; i++) {
			Pos aux = pos_balrog + Dir(i);
			if (pos_ok(aux)) {
				if (i%2 == 0) {
					Pos aux2 = aux + Dir(i);
					if (pos_ok(aux2)) W[aux2.i][aux2.j] = infinity;
				}
				else {
					for (int j = i-1; j <= i+1; j++) {
						Pos aux2 = aux + Dir(j%8);
						if (pos_ok(aux2)) W[aux2.i][aux2.j] = infinity;
					}
				}
			}
		}
	}

	// Treasure heatmap
	// W: weighted matrix, p: treasure position
	Mat t_heatmap(const Mat& W, const vector<Pos>& T) {

		int n = W.size();
		Mat D(n, vector<int>(n, infinity));
		vector< vector<bool> > V(n, vector<bool>(W[0].size(), false));
		priority_queue< pair<int,Pos>, vector< pair<int,Pos> >,
			greater< pair<int,Pos> > > Q;
		for (int i = 0; i < T.size(); i++) {
			Pos p = T[i];
			D[p.i][p.j] = 0;
			Q.push({0,p});
		}
		while (not Q.empty()) {
			int w = Q.top().first;
			Pos u = Q.top().second;
			Q.pop();
			if (not V[u.i][u.j]) {
				V[u.i][u.j] = true;
				for (int i = 0; i < 8; i++) {
					Pos v = u + Dir(i);
					if (pos_ok(v) and not V[v.i][v.j] and W[v.i][v.j] != infinity) {
						int c = W[v.i][v.j];
						if (cell(v).treasure) {
							D[v.i][v.j] = 0;
							Q.push({D[v.i][v.j],v});
						}
						else if (D[v.i][v.j] > D[u.i][u.j] + c) {
							D[v.i][v.j] = D[u.i][u.j] + 1 + c;
							Q.push({D[v.i][v.j],v});
						}
					}
				}
			}
		}
		return D;
	}

	// Ally-Dwarves heatmap
	// W: weighted matrix
	Mat d_heatmap(const Mat& W) {

		int n = W.size();
		Mat D(n, V(n, infinity));
		vector< vector<bool> > V(n, vector<bool>(W[0].size(), false));
		priority_queue< pair<int,Pos>, vector< pair<int,Pos> >,
			greater< pair<int,Pos> > > Q;
		vector<int> VD = dwarves(me());
		for (int i = 0; i < VD.size(); i++)
			for (int j = 0; j < 8; j += 2) {
				Pos aux = unit(VD[i]).pos + Dir(j);
				if (pos_ok(aux) and W[aux.i][aux.j] < infinity) {
					D[aux.i][aux.j] = 0;
					Q.push({0,aux});
				}
			}
		while (not Q.empty()) {
			int w = Q.top().first;
			Pos u = Q.top().second;
			Q.pop();
			if (not V[u.i][u.j]) {
				V[u.i][u.j] = true;
				for (int i = 0; i < 8; i++) {
					Pos v = u + Dir(i);
					if (pos_ok(v) and not V[v.i][v.j] and W[v.i][v.j] != infinity) {
						int c = W[v.i][v.j];
						if (D[v.i][v.j] > D[u.i][u.j] + c) {
							D[v.i][v.j] = D[u.i][u.j] + 1 + c;
							Q.push({D[v.i][v.j],v});
						}
					}
				}
			}
		}
		return D;
	}

	// Returns true if there is an enemy at pos p
	// p: position tested, DE: enemy distance matrix
	bool enemy(const Pos& p) {
		if (cell(p.i,p.j).id != -1 and unit(cell(p.i,p.j).id).player != me()) {
			if (unit(cell(p.i,p.j).id).type == Troll) return true;
			else if (unit(cell(p.i,p.j).id).type == Orc) return true;
			else if (unit(cell(p.i,p.j).id).type == Dwarf) return true;
		}
		return false;
	}

	// Returns true if the balrog is in a 2-cell radius distance
	bool balrog_near(const Pos& p) {

		Pos b = unit(balrog_id()).pos;
		for (int i = 0; i < 8; i++) {
			Pos aux = p + Dir(i);
			if(pos_ok(aux)) {
				if (aux == b) return true;
				if (i%2 == 0) {
					Pos aux2 = aux + Dir(i);
					if (pos_ok(aux2) and (aux2 == b)) return true;
				}
				else {
					for (int j = i-1; j <= i+1; j++) {
						Pos aux2 = aux + Dir(j%8);
						if (pos_ok(aux2) and (aux2 == b)) return true;
					}
				}
			}
		}
		return false;
	}

	// Returns the direction "i" of the adjacent enemy or -1 if there isn't any
	// id: id of my unit
	V e_adjacent(const int& id) {

		V E;
		Pos p = unit(id).pos;
		for (int i = 0; i < 8; i++) {
			Pos aux = p + Dir(i);
			if (pos_ok(aux)) {
				int id2 = cell(aux).id;
				if (id2 != -1 and unit(id2).player != me()) E.push_back(i);
			}
		}
		return E;
	}

	// Returns true if an ally wizard is adjacent on the XY cross
	bool wa_adjacent(const int& id) {

		Pos p = unit(id).pos;
		for (int i = 0; i < 8; i += 2) {
			Pos aux = p + Dir(i);
			if (pos_ok(aux)) {
				Cell c = cell(aux);
				if (c.id != -1 and unit(c.id).type == Wizard and
					unit(c.id).player == me()) return true;
			}
		}
		return false;
	}

	// Dwarf with avoids what's on his direction dir by going
	// as further as possible form it
	void run_dwarf_run(const int& id, const int& dir, const Mat& W) {

		int main_dir = (dir + 4)%8;
		int f_dir = dir;
		Pos m = unit(id).pos + Dir(main_dir);
		if (pos_ok(m) and W[m.i][m.j] < infinity) f_dir = main_dir;
		else {
			V ND = {1,-1,2,-2};
			for (int i = 0; i < ND.size(); i++) {
				int alt_dir = (main_dir + ND[i])%8;
				Pos aux = unit(id).pos + Dir(alt_dir);
				if (pos_ok(aux))
					if (pos_ok(m) and W[aux.i][aux.j] < W[m.i][m.j])
						f_dir = alt_dir;
			}
		}
		command(id,Dir(f_dir));
	}

	// Wizard with id id avoids what's on the position of the unit + dir by
	// going as further as possible form it
	void run_wizard_run(const int& id, const int& dir, const Mat& W) {

		int new_dir = (dir + 4)%8;
		Pos p;
		if ((new_dir)%2 == 0) {
			p = unit(id).pos + Dir(new_dir);
			if (pos_ok(p) and W[p.i][p.j] < infinity) {
				command(id, Dir(new_dir));
				return;
			}
			else {
				new_dir = (new_dir + 2)%8;
				p = unit(id).pos + Dir(new_dir);
				if (pos_ok(p) and W[p.i][p.j] < infinity) {
					command(id, Dir(new_dir));
					return;
				}
				new_dir = (new_dir - 2)%8;
				p = unit(id).pos + Dir(new_dir);
				if (pos_ok(p) and W[p.i][p.j] < infinity) {
					command(id, Dir(new_dir));
					return;
				}
			}
		}
		else {
			new_dir = (new_dir + 1)%8;
			p = unit(id).pos + Dir(new_dir);
			if (pos_ok(p) and W[p.i][p.j] < infinity) {
				command(id, Dir(new_dir));
				return;
			}
			new_dir = (new_dir - 1)%8;
			p = unit(id).pos + Dir(new_dir);
			if (pos_ok(p) and W[p.i][p.j] < infinity) {
				command(id, Dir(new_dir));
				return;
			}
		}
	}

	// Main function to move dwarves
	void move_dwarves(const Mat& DT, const Mat& W) {

		V vd = dwarves(me());
		int n = vd.size();
		for (int i = 0; i < n; i++) {
			int id = vd[i];
			int d;
			int min = infinity;
			for (int j = 0; j < 8; j++) {
				Pos p = unit(id).pos + Dir(j);
				if (pos_ok(p) and DT[p.i][p.j] < min and W[p.i][p.j] != infinity) {
					min = DT[p.i][p.j];
					d = j;
				}
				else if (pos_ok(p) and DT[p.i][p.j] == min and
					W[p.i][p.j] != infinity and cell(p.i,p.j).owner != me()) {
						min = DT[p.i][p.j];
						d = j;
				}
			}
			if (min == 0) command(id, Dir(d));
			else if (wa_adjacent(id)) {
				V EA = e_adjacent(id);
				if (EA.size() == 0) command(id,Dir(d));
				else {
					Cell victim = cell(unit(id).pos + Dir(EA[0]));
					int dir_v = EA[0];
					for (int i = 1; i < EA.size(); i++) {
						Cell c = cell(unit(id).pos + Dir(EA[i]));
						if (unit(c.id).health < unit(victim.id).health) {
							victim = c;
							dir_v = EA[i];
						}
					}
					if (unit(victim.id).type == Troll) run_dwarf_run(id,dir_v,W);
					else command(id,Dir(dir_v));
				}
			}
			else {
				V EA = e_adjacent(id);
				if (EA.size() == 0) command(id,Dir(d));
				else {
					Cell victim = cell(unit(id).pos + Dir(EA[0]));
					int dir_v = EA[0];
					for (int i = 1; i < EA.size(); i++) {
						Cell c = cell(unit(id).pos + Dir(EA[i]));
						if (unit(c.id).health < unit(victim.id).health) {
							victim = c;
							dir_v = EA[i];
						}
					}
					command(id,Dir(dir_v));
				}
			}
		}
	}

	// Main function to move wizards
	void move_wizards(const Mat& DD, const Mat& W) {

		V VW = wizards(me());
		int n = VW.size();
		for (int i = 0; i < n; i++) {
			int id = VW[i];
			int d;
			int min = infinity;
			if (DD[unit(id).pos.i][unit(id).pos.j] != 0) {
				for (int j = 0; j < 8; j += 2) {
					Pos p = unit(id).pos + Dir(j);
					if (pos_ok(p) and DD[p.i][p.j] < min and W[p.i][p.j] != infinity) {
						min = DD[p.i][p.j];
						d = j;
					}
				}
				V EA = e_adjacent(id);
				if (EA.size() == 0) command(id,Dir(d));
				else {
						Cell danger = cell(unit(id).pos + Dir(EA[0]));
						int dir_d = 0;
						for (int i = 1; i < EA.size(); i++) {
							Cell c = cell(unit(id).pos + Dir(EA[i]));
							if (unit(c.id).health > unit(danger.id).health) {
								danger = c;
								dir_d = i;
							}
						}
						run_wizard_run(id, dir_d, W);
				}
			}
		}
	}

  	/**
   	* Play method, invoked once per each round.
   	*/
	virtual void play () {

			Mat WD(60, V(60));
			Mat WW(60, V(60));
			vector<Pos> T;
			dwarf_weights_and_treasures(WD,T);

			Mat DT(60, V(60));
			Mat DD(60, V(60));

			DT = t_heatmap(WD,T);
			DD = d_heatmap(WW);

			move_dwarves(DT, WD);

			DD = d_heatmap(WW);

			move_wizards(DD, WW);
  	}
};


	/**
	 * Do not modify the following line.
	 */
	RegisterPlayer(PLAYER_NAME);
