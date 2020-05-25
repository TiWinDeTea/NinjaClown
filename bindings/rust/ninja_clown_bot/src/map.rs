use ninja_clown_bot_sys::{nnj_api, nnj_cell, nnj_cell_kind, nnj_cell_pos};
use std::fmt;

#[derive(Clone, Debug, Copy)]
#[repr(u32)]
pub enum CellKind {
    Unknown = nnj_cell_kind::CT_UNKNOWN.0,
    Chasm = nnj_cell_kind::CT_CHASM.0,
    Ground = nnj_cell_kind::CT_GROUND.0,
    Wall = nnj_cell_kind::CT_WALL.0,
}

impl CellKind {
    /// # Safety
    /// Calling this function with a value not representable by CellKind
    /// is undefined behavior.
    unsafe fn from_u32_unchecked(v: u32) -> Self {
        std::mem::transmute(v)
    }
}

#[derive(Clone, Debug)]
#[repr(transparent)]
pub struct Cell(nnj_cell);

impl Cell {
    pub fn kind(&self) -> CellKind {
        unsafe {
            // # Safety
            // Kind should be a valid CellKind value at any given time
            debug_assert!(self.0.kind.0 <= nnj_cell_kind::CT_WALL.0);
            CellKind::from_u32_unchecked(self.0.kind.0)
        }
    }
}

#[derive(Clone, Debug)]
#[repr(transparent)]
pub struct CellPos(nnj_cell_pos);

#[derive(Clone, Debug)]
pub struct Map {
    pub grid: Vec<Cell>, // FIXME: shouldn't be public
    width: usize,
    height: usize,
}

impl Map {
    pub fn new(raw: &nnj_api) -> Self {
        let width;
        let height;

        let grid = unsafe {
            width = (raw.map_width.unwrap())(raw.ninja_descriptor) as usize;
            height = (raw.map_height.unwrap())(raw.ninja_descriptor) as usize;

            let mut grid = Vec::new();
            grid.resize_with(width * height, nnj_cell::default);
            (raw.map_scan.unwrap())(raw.ninja_descriptor, grid.as_mut_ptr());
            std::mem::transmute(grid)
        };

        Self { grid, width, height }
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

    pub fn width(&self) -> usize {
        self.width
    }

    pub fn height(&self) -> usize {
        self.height
    }
}

impl fmt::Display for Map {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}x{}", self.width, self.height)?;
        for (i, cell) in self.grid.iter().enumerate() {
            if i % self.width == 0 {
                writeln!(f)?;
            }

            match cell.kind() {
                CellKind::Unknown => write!(f, ".")?,
                CellKind::Chasm => write!(f, "O")?,
                CellKind::Ground => write!(f, " ")?,
                CellKind::Wall => write!(f, "#")?,
            }
        }
        Ok(())
    }
}
