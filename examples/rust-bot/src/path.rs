use ninja_clown_bot::{map::CellKind, Map};
use pathfinding::prelude::{absdiff, astar};
use std::collections::HashMap;

#[derive(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct Pos(pub usize, pub usize);

impl Pos {
    pub fn dist(&self, other: &Pos) -> usize {
        absdiff(self.0, other.0) + absdiff(self.1, other.1)
    }

    pub fn center_x(&self) -> f32 {
        (self.0 as f32) + 0.5
    }

    pub fn center_y(&self) -> f32 {
        (self.1 as f32) + 0.5
    }
}

#[derive(Clone, Debug)]
pub struct PathGraph {
    successors: HashMap<Pos, Vec<Pos>>,
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
        for (i, cell) in map.grid.iter().enumerate() {
            let column = i % map.width();
            let line = i / map.width();

            if let CellKind::Ground = cell.kind() {
                let pos = Pos(column, line);
                let mut successors = Vec::new();

                for (x, y) in [
                    (column.saturating_sub(1), line),
                    (column + 1, line),
                    (column, line + 1),
                    (column, line.saturating_sub(1)),
                ]
                .iter()
                {
                    if let Some(CellKind::Ground) = map.cell_at(*x, *y).map(|c| c.kind()) {
                        successors.push(Pos(*x, *y));
                    }
                }

                self.successors.insert(pos, successors);
            }
        }
    }

    pub fn path_to(&self, start: &Pos, goal: &Pos) -> Option<Vec<Pos>> {
        astar(
            start,
            |p| {
                self.successors
                    .get(p)
                    .map(|v| v.iter().cloned().map(|p| (p, 1)).collect())
                    .unwrap_or_else(Vec::new)
            },
            |p| p.dist(goal) / 3,
            |p| p.eq(goal),
        )
        .map(|(path, _)| path)
    }
}
