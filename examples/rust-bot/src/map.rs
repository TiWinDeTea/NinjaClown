use crate::ffi::{Cell, CellType, RawApi};
use crate::map::path::PathGraph;
use std::fmt;

#[derive(Clone, Debug)]
pub struct Map {
    grid: Vec<Cell>,
    width: usize,
    height: usize,
}

impl Map {
    pub fn new(raw: &RawApi) -> Self {
        let width = unsafe { (raw.map_width)(raw.ninja_descriptor) };
        let height = unsafe { (raw.map_height)(raw.ninja_descriptor) };
        let mut grid = Vec::new();
        grid.resize_with(width * height, Cell::default);
        unsafe { (raw.map_scan)(raw.ninja_descriptor, grid.as_mut_ptr()) };

        Self {
            grid,
            width,
            height,
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut Cell {
        self.grid.as_mut_ptr()
    }

    pub fn cell_at(&self, column: usize, line: usize) -> Option<&Cell> {
        if column >= self.width || line >= self.height {
            None
        } else {
            Some(&self.grid[column + line * self.width])
        }
    }

    pub fn build_path_graph(&self) -> PathGraph {
        PathGraph::build(self)
    }
}

impl fmt::Display for Map {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}x{}", self.width, self.height)?;
        for (i, cell) in self.grid.iter().enumerate() {
            if i % self.width == 0 {
                writeln!(f)?;
            }

            match cell.typ {
                CellType::Unknown => write!(f, ".")?,
                CellType::Chasm => write!(f, "O")?,
                CellType::Ground => write!(f, " ")?,
                CellType::Wall => write!(f, "#")?,
            }
        }
        Ok(())
    }
}

pub mod path {
    use crate::ffi::CellType;
    use crate::map::Map;
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
                let column = i % map.width;
                let line = i / map.width;

                if let CellType::Ground = cell.typ {
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
                        if let Some(CellType::Ground) = map.cell_at(*x, *y).map(|c| c.typ) {
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
}
