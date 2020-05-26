use ninja_clown_bot::{
    map::{CellKind, CellPos},
    Map,
};
use pathfinding::prelude::{absdiff, astar};
use std::collections::HashMap;

fn pos_dist(a: &CellPos, b: &CellPos) -> usize {
    absdiff(a.column(), b.column()) + absdiff(a.line(), b.line())
}

#[derive(Clone, Debug)]
pub struct PathGraph {
    successors: HashMap<CellPos, Vec<CellPos>>,
}

impl PathGraph {
    pub fn build(map: &Map) -> Self {
        let mut graph = Self {
            successors: HashMap::new(),
        };
        graph.rebuild(map);
        graph
    }

    pub fn rebuild(&mut self, map: &Map) {
        for cell in map.iter_pos() {
            if let CellKind::Ground = cell.kind() {
                let mut successors = Vec::new();

                for (x, y) in [
                    (cell.column().saturating_sub(1), cell.line()),
                    (cell.column() + 1, cell.line()),
                    (cell.column(), cell.line() + 1),
                    (cell.column(), cell.line().saturating_sub(1)),
                ]
                .iter()
                {
                    if let Some(CellKind::Ground) = map.cell_at(*x, *y).map(|c| c.kind()) {
                        successors.push(CellPos::new(*x, *y));
                    }
                }

                self.successors.insert(cell.pos, successors);
            }
        }
    }

    pub fn path_to(&self, start: &CellPos, goal: &CellPos) -> Option<Vec<CellPos>> {
        astar(
            start,
            |p| {
                self.successors
                    .get(p)
                    .map(|v| v.iter().cloned().map(|p| (p, 1)).collect())
                    .unwrap_or_else(Vec::new)
            },
            |p| pos_dist(p, goal) / 3,
            |p| p.eq(goal),
        )
        .map(|(path, _)| path)
    }
}
